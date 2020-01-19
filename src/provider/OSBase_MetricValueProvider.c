/*
 * $Id: OSBase_MetricValueProvider.c,v 1.16 2012/05/17 01:02:42 tyreld Exp $
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
 * Description: Generic Provider for Metric Values served by
 *              the SBLIM Gatherer.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <metric.h>
#include <rrepos.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "OSBase_MetricUtil.h"

#ifdef DEBUG
#define _debug 1
#else
#define _debug 0
#endif

const CMPIBroker * _broker;

/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

static char * _ClassName = "CIM_BaseMetricValue";
static char * _FILENAME = "OSBase_MetricValueProvider.c";

#ifdef CMPI_VER_100
#define OSBase_MetricValueProviderSetInstance OSBase_MetricValueProviderModifyInstance 
#endif

static char * metricValueName(const CMPIObjectPath *path);

static void returnPathes(const CMPIBroker *_broker,
			 const CMPIContext *ctx,
			 RepositoryPluginDefinition *rdef,
			 int rdefnum,
			 COMMHEAP *ch,
			 const CMPIObjectPath *ref,
			 const CMPIResult *rslt,
			 CMPIStatus *rc);

static void returnInstances(const CMPIBroker *_broker,
			    const CMPIContext *ctx,
			    RepositoryPluginDefinition *rdef,
			    int rdefnum,
			    COMMHEAP *ch,
			    const CMPIObjectPath *ref,
			    const CMPIResult *rslt,
                            const char ** props,
			    CMPIStatus *rc);

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_MetricValueProviderCleanup( CMPIInstanceMI * mi, 
					      const CMPIContext * ctx, 
					      CMPIBoolean terminate) { 
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI Cleanup()\n", _FILENAME, _ClassName );
  releaseMetricDefClasses();
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricValueProviderEnumInstanceNames( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref) { 
  CMPIStatus       rc = {CMPI_RC_OK, NULL};
  int              rdefnum;
  RepositoryPluginDefinition *rdef;
  COMMHEAP         ch;
  char           **pnames;
  int              pnum;
  int              i;
  
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstanceNames()\n", _FILENAME, _ClassName );

  if (checkRepositoryConnection()) {
    /* get list of metric ids */
    ch=ch_init();
    pnum = getPluginNamesForValueClass(_broker,ctx,ref,&pnames);
    for (i=0; i<pnum; i++) {
      rdefnum = rreposplugin_list(pnames[i],&rdef,ch);
      returnPathes(_broker,ctx,rdef,rdefnum,ch,ref,rslt,&rc);
    }
    releasePluginNames(pnames);
    ch_release(ch);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Gatherer Service not active" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricValueProviderEnumInstances( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) { 
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  int              rdefnum;
  RepositoryPluginDefinition *rdef;
  COMMHEAP         ch;
  char           **pnames;
  int              pnum;
  int              i;

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstances()\n", _FILENAME, _ClassName );

  if (checkRepositoryConnection()) {
    /* get list of metric ids */
    ch=ch_init();
    pnum = getPluginNamesForValueClass(_broker,ctx,ref,&pnames);
    for (i=0; i<pnum; i++) {
      rdefnum = rreposplugin_list(pnames[i],&rdef,ch);
      returnInstances(_broker,ctx,rdef,rdefnum,ch,ref,rslt,properties,&rc);
    }
    releasePluginNames(pnames);
    ch_release(ch);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Gatherer Service not active" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricValueProviderGetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop, 
           const char ** properties) {
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  ValueRequest     vr;
  COMMHEAP         ch;
  int              vId;
  char             vName[300];
  char             vResource[300];
  char             vSystemId[300];
  time_t           vTimestamp;

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI GetInstance()\n", _FILENAME, _ClassName );
  
  if (checkRepositoryConnection()) {
    ch=ch_init();
    if (parseMetricValueId(metricValueName(cop),vName,&vId,vResource,vSystemId,&vTimestamp) 
	== 0) {
      if( _debug )
	fprintf(stderr, "id criteria %s,%s, %ld\n", vName, vResource, 
		vTimestamp);
      // get value 
      vr.vsId = vId;
      vr.vsResource = vResource;
      vr.vsSystemId = vSystemId;
      vr.vsFrom = vr.vsTo = vTimestamp;
      if (rrepos_get(&vr,ch)== 0) {
	if( _debug )
	  fprintf( stderr, "::: got %d values for id \n", vr.vsId);
	if (vr.vsNumValues>=1) {
	  ci = makeMetricValueInst( _broker, ctx, vName, vId, &vr.vsValues[0], vr.vsDataType,
				    cop, properties, &rc );    
	  if( ci == NULL ) {
	    if( _debug ) {
	      if( rc.msg != NULL )
		{ fprintf(stderr,"rc.msg: %s\n",CMGetCharPtr(rc.msg)); }
	    }
	    if (rc.rc == CMPI_RC_OK) {
	      /* it can't be OK if we didn't get an instance */
	      CMSetStatusWithChars( _broker, &rc, 
				    CMPI_RC_ERR_NOT_FOUND, 
				    "Invalid metric value id" ); 
	    }
	  } else {
	    CMReturnInstance( rslt, ci );
	  }
	} else {
	  CMSetStatusWithChars( _broker, &rc, 
				CMPI_RC_ERR_NOT_FOUND, 
				"no values returned by Gatherer repository" ); 
	}
      } else {
	CMSetStatusWithChars( _broker, &rc, 
			      CMPI_RC_ERR_NOT_FOUND, 
			      "Gatherer repository reported error" ); 
      }
    } else {
      CMSetStatusWithChars( _broker, &rc, 
			    CMPI_RC_ERR_INVALID_PARAMETER, "Invalid Object Path Key \"Id\""); 
    }
    ch_release(ch);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Gatherer Service not active" ); 
  }
  CMReturnDone(rslt);
  return rc;
}

CMPIStatus OSBase_MetricValueProviderCreateInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop, 
           const CMPIInstance * ci) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI CreateInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricValueProviderSetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop,
           const CMPIInstance * ci, 
           const char **properties) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI SetInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricValueProviderDeleteInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop) {
  CMPIStatus rc = {CMPI_RC_OK, NULL}; 

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI DeleteInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricValueProviderExecQuery( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char * lang, 
           const char * query) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI ExecQuery()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}



/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMInstanceMIStub( OSBase_MetricValueProvider, 
                  OSBase_MetricValueProvider, 
                  _broker, 
                  CMNoHook);

/* ---------------------------------------------------------------------------*/
/*                               Private Stuff                                */
/* ---------------------------------------------------------------------------*/


static char * metricValueName(const CMPIObjectPath *path)
{
  CMPIData clsname = CMGetKey(path,"InstanceId",NULL);
  if (clsname.value.string)
    return CMGetCharPtr(clsname.value.string);
  else
    return NULL;
}

static void returnPathes(const CMPIBroker *_broker,
			 const CMPIContext *ctx,
			 RepositoryPluginDefinition *rdef,
			 int rdefnum,
			 COMMHEAP *ch,
			 const CMPIObjectPath *ref,
			 const CMPIResult *rslt,
			 CMPIStatus *rc)
{
  int i,j;
  ValueRequest vr;
  CMPIObjectPath * co = NULL;
  
  for (i=0; i < rdefnum; i++) {
    vr.vsId = rdef[i].rdId;
    vr.vsResource = NULL;
    vr.vsSystemId = NULL;
    vr.vsFrom = vr.vsTo = 0;
    vr.vsNumValues=1; /* restrict to one/latest value per resource */
    if( _debug )
      fprintf( stderr, "::: getting value for id %d\n", rdef[i].rdId);
    if (rrepos_get(&vr,ch) == 0) {
      if( _debug )
	fprintf( stderr, "::: got %d values for id \n", vr.vsId);
      for (j=0; j< vr.vsNumValues; j++) {
	co = makeMetricValuePath(_broker, ctx, rdef[i].rdName, rdef[i].rdId,
				 &vr.vsValues[j], 
				 ref, rc);	
	if( co == NULL ) {
	  if( _debug ) {
	    if( rc->msg != NULL )
	      { fprintf(stderr,"rc.msg: %s\n",CMGetCharPtr(rc->msg)); }
	  }
	  break;
	} else {
	  CMReturnObjectPath( rslt, co );
	}
      }
    }
  }
}

static void returnInstances(const CMPIBroker *_broker,
			    const CMPIContext *ctx,
			    RepositoryPluginDefinition *rdef,
			    int rdefnum,
			    COMMHEAP *ch,
			    const CMPIObjectPath *ref,
			    const CMPIResult *rslt,
                            const char ** props,
			    CMPIStatus *rc)
{
  int i,j;
  ValueRequest vr;
  CMPIInstance * ci = NULL;
  
  for (i=0; i < rdefnum; i++) {
    vr.vsId = rdef[i].rdId;
    vr.vsResource = NULL;
    vr.vsSystemId = NULL;
    vr.vsFrom = vr.vsTo = 0;
    vr.vsNumValues=1; /* restrict to one/latest value per resource */
    if( _debug )
      fprintf( stderr, "::: getting value for id %d\n", rdef[i].rdId);
    if (rrepos_get(&vr,ch) == 0) {
      if( _debug )
	fprintf( stderr, "::: got %d values for id \n", vr.vsId);
      for (j=0; j< vr.vsNumValues; j++) {
	ci = makeMetricValueInst( _broker, ctx, rdef[i].rdName, rdef[i].rdId,
				  &vr.vsValues[j], vr.vsDataType,
				  ref, props, rc );
	if( ci == NULL ) {
	  if( _debug ) {
	    if( rc->msg != NULL )
	      { fprintf(stderr,"rc.msg: %s\n",CMGetCharPtr(rc->msg)); }
	  }
	  break;
	} else {      
	  CMReturnInstance( rslt, ci );
	}
      }
    }
  }
}

/* ---------------------------------------------------------------------------*/
/*              end of OSBase_MetricValueProvider                             */
/* ---------------------------------------------------------------------------*/

