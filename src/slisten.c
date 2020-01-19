/*
 * $Id: slisten.c,v 1.7 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2004, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Subscription Forwarding
 * 
 */

#include "slisten.h"
#include "marshal.h"
#include "mtrace.h"
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKFILE_TEMPLATE "/tmp/rrepos-socket-XXXXXX"


static pthread_t pt_listener;
static char listener[] = SOCKFILE_TEMPLATE;
static int  fdsockfile = -1;

typedef struct _CallbackEntry {
  int                    cb_metricid;
  int                    cb_corrid;
  SubscriptionCallback * cb_callback;
  struct _CallbackEntry *cb_next;
} CallbackEntry;

static CallbackEntry * cbHead = NULL;

static void call_callbacks(int corrid, ValueRequest *vr)
{
  CallbackEntry * cbl = cbHead;
  while(cbl && corrid <= cbl->cb_corrid) {
    if (cbl->cb_corrid == corrid && cbl->cb_metricid == vr->vsId ) {
      cbl->cb_callback(corrid,vr);
    }
    cbl = cbl->cb_next;
  }
}

static void subs_listener_exit(void)
{
  /* avoids socket file ruins in /tmp */
  unlink(listener);
}

static void subs_listener_cleanup(void *fdsocket)
{
  /* reset to initial state */
  long fds = (long)fdsocket;
  close(fds);
  unlink(listener);
  strcpy(listener,SOCKFILE_TEMPLATE);
  fdsockfile=-1;
}

static void * subs_listener(void *unused)
{
  long               fdsocket;
  struct sockaddr_un sockname;
  void              *v_corrid;
  ValueRequest      *vr;
  char               buf[1000];
  off_t              offset;

  M_TRACE(MTRACE_FLOW,MTRACE_RREPOS,
	  ("subs_listener"));
  fdsocket = socket(AF_UNIX,SOCK_DGRAM,0);
  pthread_cleanup_push(subs_listener_cleanup,(void*)fdsocket);
  if (fdsocket != -1) {
    atexit(subs_listener_exit);
    sockname.sun_family = AF_UNIX;
    strcpy(sockname.sun_path,listener);
    if (bind(fdsocket,(struct sockaddr*)&sockname,sizeof(sockname))==-1) {
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("subs_listener bind error (%s: %s)",strerror(errno),
	       sockname.sun_path));
    }
    while(1) {
      offset=0;
      if (recv(fdsocket,buf,sizeof(buf),0) != -1) {
	if (unmarshal_data(&v_corrid,sizeof(int),
			   buf,&offset,sizeof(buf)) == 0 &&
	    unmarshal_valuerequest(&vr,buf,&offset,
				   sizeof(buf)) == 0) {
	  M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		  ("subs_listener received a metric packet for %d",vr->vsId));
	  call_callbacks(*(int*)v_corrid,vr);
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		  ("subs_listener unmmarshalling error"));
	}	  
      } else {
	if (errno==EINTR) {
	  M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		  ("subs_listener interrupted"));
	  continue;
	}
	M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		("subs_listener recv error (%s)",strerror(errno)));
	break;
      }
    }
    /* clean up after an error occurs */
  } else {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("subs_listener socket error (%s)",strerror(errno)));
    /* socket error */
  }

  pthread_cleanup_pop(1);
  return NULL;
}

int add_subscription_listener(char *listenerid, SubscriptionRequest *sr,
			      SubscriptionCallback *scb)
{
  CallbackEntry *cbl = cbHead;
  CallbackEntry *prev = cbHead;
  M_TRACE(MTRACE_FLOW,MTRACE_RREPOS,
	  ("add_subscription_listener"));
  if (fdsockfile==-1) {
    fdsockfile=mkstemp(listener);
    M_TRACE(MTRACE_DETAILED,MTRACE_RREPOS,
	    ("listener socket name = %s",listener));
    if (fdsockfile != -1) {
      close(fdsockfile);
      unlink(listener);
      pthread_create(&pt_listener,NULL,subs_listener,NULL);
      pthread_detach(pt_listener);
    } else {
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("could not create listener socket file %s",listener));
      return -1;
    }
  }
  strcpy(listenerid,listener);
  while (cbl) {
    if (cbl->cb_metricid == sr->srMetricId && 
	cbl->cb_corrid == sr->srCorrelatorId &&
	cbl->cb_callback == scb) {
      /* already there */
      M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
	      ("already enabled metric id %d for listening", sr->srMetricId));
      return 0;
    } else if (cbl->cb_next && cbl->cb_next->cb_corrid > sr->srCorrelatorId) {
      break;
    }
    prev = cbl;
    cbl = cbl->cb_next;
  }
  M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
	  ("enabling metric id %d for listening", sr->srMetricId));
  cbl = malloc(sizeof(CallbackEntry));
  cbl->cb_metricid = sr->srMetricId;
  cbl->cb_corrid = sr->srCorrelatorId;
  cbl->cb_callback = scb;
  if (cbHead == prev && 
      (cbHead == NULL || cbHead->cb_corrid > sr->srCorrelatorId) ) {
    cbl->cb_next = cbHead;
    cbHead = cbl;
  } else {
    cbl->cb_next = prev->cb_next;
    prev->cb_next = cbl;
  }
  return 0;
}

int remove_subscription_listener(const char *listenerid, 
				 SubscriptionRequest *sr,
				 SubscriptionCallback *scb)
{
  CallbackEntry *cbl = cbHead;
  CallbackEntry *prev = cbHead;
  M_TRACE(MTRACE_FLOW,MTRACE_RREPOS,
	  ("remove_subscription_listener"));
  if (strcmp(listenerid,listener)) {
    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	    ("listener %s is not active",listenerid));
    return -1;
  }
  while(cbl && sr->srCorrelatorId <= cbl->cb_corrid) {
    if (cbl->cb_corrid == sr->srCorrelatorId && 
	cbl->cb_metricid == sr->srMetricId && 
	cbl->cb_callback == scb) {
      /* unlink */
      if (prev == cbl) {
	cbHead = cbl->cb_next;
      } else {
	prev->cb_next = cbl->cb_next;
      }
      free(cbl);
      /* stop listening ? */
      return 0;
    }
    prev = cbl;
    cbl = cbl->cb_next;
  }
  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	  ("subscription request (%d,%d) not found",
	   sr->srMetricId,sr->srCorrelatorId));
  return -1;
}

int current_subscription_listener(char *listenerid)
{
  strcpy(listenerid,listener);
  return 0;
}
