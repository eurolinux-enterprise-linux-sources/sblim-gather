
/*
 * $Id: OSBase_MetricLifeCycleProvider.c,v 1.4 2012/05/17 01:02:42 tyreld Exp $
 *
 * © Copyright IBM Corp. 2003, 2007, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors:
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: Provider for the Metric Indications.
 *
 */

#ifdef DEBUG
#define _debug 1
#else
#define _debug 0
#endif


#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <mtrace.h>
#include <rrepos.h>
#include "OSBase_MetricUtil.h"

static const CMPIBroker * _broker;

/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

static char * _ClassName = "CIM_InstModification";
static char * _FILENAME = "OSBase_MetricLifeCycleProvider.c";

static pthread_mutex_t listenMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t   listen_key;
static pthread_once_t  listen_once = PTHREAD_ONCE_INIT;
static CMPIContext   * listenContext = NULL;

static int enabled = 0;
static int runningCorrelator = 100;

static int responsible(const CMPISelectExp * filter,const CMPIObjectPath * op, SubscriptionRequest * sr);
static int addListenFilter(const CMPISelectExp * filter, const CMPIObjectPath * op, SubscriptionRequest *sr);
static int removeListenFilter(const CMPISelectExp * filter);
static void enableFilters();
static void disableFilters();

typedef struct _ListenFilter {
  int                    lf_enabled;
  CMPISelectExp *        lf_filter;
  SubscriptionRequest  * lf_subs;
  char                 * lf_namespace;
  struct _ListenFilter * lf_next;
} ListenFilter;
static ListenFilter * listenFilters = NULL;

static int subscribeFilter(ListenFilter *lf);
static int unsubscribeFilter(ListenFilter *lf);
static void metricIndicationCB(int corrid, ValueRequest *vr);

static void listen_term(void *voidctx);
static void listen_init();
static CMPIContext * attachListenContext();


/* ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/*                      Indication Provider Interface                             */
/* ---------------------------------------------------------------------------*/
CMPIStatus OSBase_MetricLifeCycleProviderIndicationCleanup( CMPIIndicationMI * mi, 
							     const CMPIContext * ctx,
							     CMPIBoolean terminate) 
{
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI IndicationCleanup()\n", _FILENAME, _ClassName );

  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricLifeCycleProviderMustPoll
(CMPIIndicationMI * mi, 
 const CMPIContext * ctx, 
#ifndef CMPI_VER_100
 const CMPIResult * res, 
#endif
 const CMPISelectExp * filter,
 const char * evtype, 
 const CMPIObjectPath * co)
{
  if( _debug )
    fprintf(stderr,"*** not polling for %s\n", _ClassName);
#ifndef CMPI_VER_100
  CMReturnData(res, &_false, CMPI_boolean);
  CMReturnDone(res);
#endif
#ifndef CMPI_VER_100
  CMReturn(CMPI_RC_OK);
#else
  CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#endif
}

CMPIStatus OSBase_MetricLifeCycleProviderAuthorizeFilter
(CMPIIndicationMI * mi, 
 const CMPIContext * ctx, 
#ifndef CMPI_VER_100
 const CMPIResult * res, 
#endif
 const CMPISelectExp * filter,
 const char * evtype, 
 const CMPIObjectPath * co, 
 const char * owner)
{
  if (responsible(filter,co,NULL)) {
    if( _debug )
      fprintf(stderr,"*** successfully authorized filter for %s\n", _ClassName);
#ifndef CMPI_VER_100
    CMReturnData(res, &_true, CMPI_boolean);
#else
    CMReturn(CMPI_RC_OK);
#endif
  } else {
    if( _debug )
      fprintf(stderr,"*** refused to authorize filter for %s\n", _ClassName);
#ifndef CMPI_VER_100
    CMReturnData(res, &_false, CMPI_boolean);
#else
  CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#endif
  }
#ifndef CMPI_VER_100
  CMReturnDone(res);
#endif
#ifndef CMPI_VER_100
  CMReturn(CMPI_RC_OK);
#else
  CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#endif
}

CMPIStatus OSBase_MetricLifeCycleProviderActivateFilter
(CMPIIndicationMI * mi, 
 const CMPIContext * ctx, 
#ifndef CMPI_VER_100
 const CMPIResult * res, 
#endif
 const CMPISelectExp * filter,
 const char * evtype, 
 const CMPIObjectPath * co, 
 CMPIBoolean first)
{
  SubscriptionRequest * sr = calloc(1,sizeof(SubscriptionRequest));
  if (responsible(filter,co,sr)) {
    /* Prepare attachment of secondary threads */
    if (listenContext == NULL) {
      if( _debug )
	fprintf(stderr,"*** preparing seondary thread attach\n");
      listenContext = CBPrepareAttachThread(_broker,ctx);
    }
    if (addListenFilter(filter,co,sr)== 0) {
      if( _debug )
	fprintf(stderr,"*** successfully activated filter for %s\n", _ClassName);
      CMReturn(CMPI_RC_OK);
    } else {
      /* was not freed in addListenFilter */
      free(sr);
    }
  }
  if( _debug )
    fprintf(stderr,"*** could not activate filter for %s\n", _ClassName);
  CMReturn(CMPI_RC_ERR_FAILED);
}

CMPIStatus OSBase_MetricLifeCycleProviderDeActivateFilter
(CMPIIndicationMI * mi, 
 const CMPIContext * ctx,
#ifndef CMPI_VER_100
 const CMPIResult * res,
#endif
 const CMPISelectExp * filter,
 const char * evtype,
 const CMPIObjectPath * co,
 CMPIBoolean last)
{
  if (responsible(filter,co,NULL) && removeListenFilter(filter) == 0) {
    if( _debug )
      fprintf(stderr,"*** successfully deactivated filter for %s\n", _ClassName);
    CMReturn(CMPI_RC_OK);
  } else {
    if( _debug )
      fprintf(stderr,"*** could not deactivate filter for %s\n", _ClassName);
    CMReturn(CMPI_RC_ERR_FAILED);
  }
}

CMPIStatus OSBase_MetricLifeCycleProviderEnableIndications
(CMPIIndicationMI * mi, const CMPIContext * ctx)
{
  enableFilters();
  if( _debug )
    fprintf(stderr,"*** successfully enabled indications for %s\n", _ClassName);
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricLifeCycleProviderDisableIndications
(CMPIIndicationMI * mi, const CMPIContext * ctx)
{
  disableFilters();
  if( _debug )
    fprintf(stderr,"*** successfully disabled indications for %s\n", _ClassName);
  CMReturn(CMPI_RC_OK);
}


/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

static void init_trace() {
#ifndef NOTRACE
  if (_debug)
    fprintf(stderr, "*** initialize provider tracing\n");
  m_trace_setlevel(4);
  m_trace_setfile("/tmp/repos_provider.trc");
  m_trace_enable(MTRACE_MASKALL);
#endif
}

CMIndicationMIStub( OSBase_MetricLifeCycleProvider, 
		    OSBase_MetricLifeCycleProvider, 
		    _broker, 
		    init_trace());


/* ---------------------------------------------------------------------------*/
/*                               Private Stuff                                */
/* ---------------------------------------------------------------------------*/

static int responsible(const CMPISelectExp * filter, const CMPIObjectPath *op, SubscriptionRequest * sr)
{
  if (op && filter) {
    CMPISelectCond * cond = CMGetDoc(filter,NULL);
    CMPIString     * condstring = filter->ft->getString(filter,NULL);
    /* only supporting instance modifications */
    if (!CMClassPathIsA(_broker,op,_ClassName,NULL)) {
      fprintf (stderr,"*** class path = %s\n",
	       CMGetCharPtr(CDToString(_broker,op,NULL)));
      return 0;
    }
    if (condstring && cond) {
      size_t i;
      int j;
      CMPICount scount = CMGetSubCondCountAndType(cond,NULL,NULL);
      char * condarr = CMGetCharPtr(condstring);
      if (_debug)
	fprintf (stderr,"*** select expr = %s\n", condarr);
      if (_debug)
	fprintf (stderr, "*** got %d sub condition(s)\n",scount);
      for (i=0; i<scount; i++) {
	CMPISubCond * subcond = CMGetSubCondAt(cond,i,NULL);
	if (subcond) {
	  CMPICount pcount = CMGetPredicateCount(subcond,NULL);
	  if (_debug)
	    fprintf (stderr, "*** got %d predicate(s)\n",pcount);
	  for (j=pcount-1; j>=0; j--) {
	    CMPIPredicate * pred = CMGetPredicateAt(subcond,j,NULL);
	    if (pred) {
	      CMPIType   type;
	      CMPIPredOp op;
	      CMPIString *lhs=NULL;
	      CMPIString *rhs=NULL;
	      char       *lhs_c=NULL;
	      char       *rhs_c=NULL;
	      if (_debug)
		fprintf (stderr, "*** checking predicate\n");
	      CMGetPredicateData(pred,&type,&op,&lhs,&rhs);
	      if (lhs) {
		/* translate to lower, allows to us strstr */
		lhs_c = strdup(CMGetCharPtr(lhs));
		char * s = lhs_c;
		while (*s) {
		  tolower(*s++);
		}
	      }
	      if (rhs) {
		rhs_c = CMGetCharPtr(rhs);
	      }
	      if (_debug)
		fprintf (stderr, "*** lhs=%s, rhs=%s\n",lhs_c, rhs_c);
	      if (strstr(lhs_c,"metricdefinitionid")==0 && 
		  op == CMPI_PredOp_Equals) {
		/* we require metric definition id to be specified */
		if (_debug)
		  fprintf (stderr, "*** maps to subscription to %s\n", rhs_c);
		if (sr && rhs_c) {
		  char name[300];
		  parseMetricDefId(rhs_c,name,&sr->srMetricId);
		  sr->srCorrelatorId = runningCorrelator++;
		}
		if (lhs_c) {
		  free(lhs_c);
		}
		return 1;
	      }
	      if (lhs_c) {
		free(lhs_c);
	      }
	    }
	  }
	}
      }
    }
  }
  return 0;
}

static int addListenFilter(const CMPISelectExp * filter, const CMPIObjectPath * op, SubscriptionRequest *sr)
{
  ListenFilter *lf;
  pthread_mutex_lock(&listenMutex);
  lf = listenFilters;
  while (lf && lf->lf_next) {
    /* assuming no double filters */
    lf = lf->lf_next;
  }
  if (listenFilters == NULL) {
    lf = listenFilters = calloc(1,sizeof(ListenFilter));
  } else {
    lf->lf_next = calloc(1,sizeof(ListenFilter));
    lf = lf->lf_next;
  }
  lf->lf_filter = (CMPISelectExp*)filter;
  lf->lf_subs = sr;
  lf->lf_namespace = strdup(CMGetCharPtr(CMGetNameSpace(op,NULL)));
  if (enabled) {
    subscribeFilter(lf);
  }
  pthread_mutex_unlock(&listenMutex);
  return 0;
}

static int removeListenFilter(const CMPISelectExp * filter)
{
  ListenFilter *lf;
  ListenFilter *prev;
  pthread_mutex_lock(&listenMutex);
  lf = listenFilters;  
  prev = lf;
  int state=1;
  while (lf) {
    if (lf->lf_filter==filter) {
      /* is the filter pointer the same as in add? */
      if (prev == listenFilters) {
	listenFilters = lf->lf_next;;
      } else {
	prev->lf_next = lf->lf_next;
      }
      if (lf->lf_enabled) {
	unsubscribeFilter(lf);
      }
      if (lf->lf_subs) {
	free (lf->lf_subs);
      }
      if (lf->lf_namespace) {
	free (lf->lf_namespace);
      }
      free(lf);
      state=0;
    }
    prev = lf;
    lf = lf->lf_next;
  }
  pthread_mutex_unlock(&listenMutex);
  return state;
}

void enableFilters()
{
  ListenFilter *lf;
  pthread_mutex_lock(&listenMutex);
  lf = listenFilters;
  while (lf) {
    if (!lf->lf_enabled) {
      if (_debug)
	fprintf (stderr, "*** enabling filter %p\n", lf->lf_filter);
      subscribeFilter(lf);
    }
    lf=lf->lf_next;
  }
  enabled = 1;
  pthread_mutex_unlock(&listenMutex);
}

void disableFilters()
{
  ListenFilter *lf;
  pthread_mutex_lock(&listenMutex);
  lf = listenFilters;
  while (lf) {
    if (lf->lf_enabled) {
      if (_debug)
	fprintf (stderr, "*** disabling filter %p\n", lf->lf_filter);
      unsubscribeFilter(lf);
    }
    lf=lf->lf_next;
  }
  enabled = 0;
  pthread_mutex_unlock(&listenMutex);
}

static int subscribeFilter(ListenFilter *lf)
{
  if (lf && lf->lf_subs && rrepos_subscribe(lf->lf_subs,metricIndicationCB)==0) {
    lf->lf_enabled = 1;
    if (_debug)
      fprintf (stderr, "*** subscribing filter %p\n", lf->lf_filter);
    return 0;
  }
  return 1;
}

static int unsubscribeFilter(ListenFilter *lf)
{
  if (lf && lf->lf_subs && rrepos_unsubscribe(lf->lf_subs,metricIndicationCB)==0) {
    lf->lf_enabled = 0;
    if (_debug)
      fprintf (stderr, "*** unsubscribing filter %p\n", lf->lf_filter);
    return 0;
  }
  return 1;
}

static void metricIndicationCB(int corrid, ValueRequest *vr)
{
  CMPIObjectPath * co;
  CMPIInstance   * ci, *mvci;
  CMPIContext    * ctx;
  ListenFilter   * lf;
  CMPIDateTime   * datetime;
  char             mName[1000];
  int              mId;
  char             mdefId[1000];
  if (_debug)
    fprintf (stderr,"*** metric indication callback triggered\n");

  ctx = attachListenContext();
  if (ctx==NULL) {
    if (_debug)
      fprintf (stderr,"*** failed to get thread context \n");
    return;
  }
  
  lf = listenFilters;
  while (lf) {
    if (lf->lf_enabled && lf->lf_subs && lf->lf_subs->srCorrelatorId==corrid) {
      break;
    }
    lf = lf->lf_next;
  }

  if (lf) {
    co = CMNewObjectPath(_broker,lf->lf_namespace,_ClassName,NULL);
    if (co && makeMetricDefIdFromCache(_broker,ctx,lf->lf_namespace,mdefId,vr->vsId)) {
      ci = CMNewInstance(_broker,co,NULL);
      if (ci) {
	datetime = 
	  CMNewDateTimeFromBinary(_broker,
				  (long long)vr->vsValues->viCaptureTime*1000000,
				  0, NULL);
	if (datetime)
	  CMSetProperty(ci,"IndicationTime",&datetime,CMPI_dateTime);

	parseMetricDefId(mdefId,mName,&mId);
	mvci = makeMetricValueInst(_broker, ctx, mName, mId, vr->vsValues, 
				   vr->vsDataType, co, NULL, NULL);
	if (mvci) {
	  CMSetProperty(ci,"SourceInstance",&mvci,CMPI_instance);
	  if (_debug)
	    fprintf (stderr,"*** delivering metric lifecycle indication\n");
	  CBDeliverIndication(_broker,ctx,lf->lf_namespace,ci);
	}
      }      
    } else { 
      if (_debug)
	fprintf (stderr,"*** failed to construct indication\n");
    }
  } else {
    if (_debug)
      fprintf (stderr,"*** received uncorrelated event\n");
  }
}

static void listen_term(void *voidctx)
{
  /* detach thread */
  CMPIContext *ctx;
  if (_debug)
    fprintf (stderr,"*** detaching thread from CMPI\n");
  ctx = (CMPIContext*)voidctx;
  CBDetachThread(_broker,ctx);
}

static void listen_init()
{
  pthread_key_create(&listen_key,listen_term);
}

static CMPIContext* attachListenContext()
{
  /* attach CMPI to current thread if required */
  CMPIContext *ctx;
  pthread_once(&listen_once,listen_init);
  ctx = (CMPIContext*)pthread_getspecific(listen_key);
  if (ctx == NULL && listenContext) {
    if (_debug)
      fprintf (stderr,"*** attaching thread to CMPI\n");
    CBAttachThread(_broker,listenContext);
    ctx = listenContext;
    pthread_setspecific(listen_key,ctx); 
  }
  return ctx;
}

/* ---------------------------------------------------------------------------*/
/*              end of OSBase_MetricLifeCycleProvider                         */
/* ---------------------------------------------------------------------------*/

