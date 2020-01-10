/*
 * $Id: sforward.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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

#include "sforward.h"
#include "marshal.h"
#include <mcclt.h>
#include <mtrace.h>
#include <mlog.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef struct _ForwardingEntry {
  int                      fw_id;
  int                      fw_corrid;
  int                      fw_origcorrid;
  struct sockaddr_un       fw_listener;
  struct _ForwardingEntry *fw_next;
} ForwardingEntry;

static ForwardingEntry * fwHead = NULL;
long   fwCorrelatorId = 0;
static int fwSocket = -1;

int subs_enable_forwarding(SubscriptionRequest *sr, const char *listenerid)
{
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	  ("subs_enable_forwarding %p %s", sr, listenerid));
  if (sr && listenerid) {
    ForwardingEntry *fwl = fwHead;
    ForwardingEntry *prev = fwHead;
    while (fwl) {
      if (fwl->fw_id == sr->srMetricId && 
	  fwl->fw_origcorrid == sr->srCorrelatorId &&
	  strcmp(listenerid,fwl->fw_listener.sun_path)==0) {
	/* already there */
	M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
		("already forwarding metric id %d for listener %s", sr->srMetricId, listenerid));
	return 0;
      }
      prev = fwl;
      fwl = fwl->fw_next;
    }    
    M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
	    ("enabling forwarding for metric id %d to listener %s", sr->srMetricId, listenerid));
    fwl = malloc(sizeof(ForwardingEntry));
    fwl->fw_id = sr->srMetricId;
    fwl->fw_corrid = fwCorrelatorId ++;
    fwl->fw_origcorrid = sr->srCorrelatorId;
    fwl->fw_listener.sun_family = AF_UNIX;
    strcpy(fwl->fw_listener.sun_path,listenerid);
    if (fwHead == NULL) {
      fwHead = fwl;
    } else {
      prev->fw_next = fwl;
    }
    fwl->fw_next=NULL;
    /* need to change to local correlator id */
    sr->srCorrelatorId = fwl->fw_corrid;
    return 0;
  }
  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	  ("subs_enable_forwarding invalid parameter %p %s", sr, listenerid));
  return -1;
}

int subs_disable_forwarding(SubscriptionRequest *sr, const char *listenerid)
{
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	  ("subs_disable_forwarding %p %s", sr, listenerid));
  if (sr && listenerid) {
    ForwardingEntry *fwl = fwHead;
    ForwardingEntry *prev = fwHead;
    while (fwl) {
      if (fwl->fw_id == sr->srMetricId && 
	  fwl->fw_origcorrid == sr->srCorrelatorId &&
	  strcmp(listenerid,fwl->fw_listener.sun_path)==0) {
	/* already there */
	M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
		("disable forwarding metric id %d for listener %s", 
		 sr->srMetricId, listenerid));
	if (prev == fwl) {
	  fwHead = fwl->fw_next;
	} else {
	  prev->fw_next = fwl->fw_next;
	}
	free(fwl);
	return 0;
      }
      prev = fwl;
      fwl = fwl->fw_next;
    }    
    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	    ("subs_disable_forwarding entry not found (%d, %d)", 
	     sr->srMetricId, sr->srCorrelatorId));
    return -1;
  }
  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	  ("subs_disable_forwarding invalid parameter %p %s", sr, listenerid));
  return -1;
}

void subs_forward(int corrid, ValueRequest *vr)
{
  char   sendbuffer[1000];
  off_t  sendsize;
  ForwardingEntry *fwl = fwHead;
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	  ("subs_forward %d %p", corrid, vr));

  while(fwl && corrid >= fwl->fw_corrid) {
    sendsize=0;
    if (fwl->fw_corrid == corrid) {
      if (marshal_data(&fwl->fw_origcorrid,sizeof(fwl->fw_origcorrid),
		       sendbuffer,
		       &sendsize,sizeof(sendbuffer)) == 0 &&
	  marshal_valuerequest(vr,sendbuffer,&sendsize
			       ,sizeof(sendbuffer)) == 0) { 
	if (fwSocket == -1) {
	  M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
		  ("opening socket for listener %s", fwl->fw_listener.sun_path));
	  fwSocket = socket(AF_UNIX,SOCK_DGRAM,0);
	}
	if (fwSocket != -1 ) {
	  M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
		  ("forwarding metric id %d to listener %s", 
		   fwl->fw_id, fwl->fw_listener.sun_path));
	  if (sendto(fwSocket, sendbuffer, sendsize, 0, 
		     (struct sockaddr*)&fwl->fw_listener, 
		     sizeof(struct sockaddr_un)) == -1 ) {
	    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		    ("send error forwarding metric id %d to %s." 
		     "System error message is %s ", 
		     fwl->fw_id, fwl->fw_listener.sun_path,
		     strerror(errno)));
	    m_log(M_ERROR, M_QUIET,"send error forwarding metric id %d to %s."
		  " System error message is %s ", 
		  fwl->fw_id, fwl->fw_listener.sun_path,strerror(errno));
	  }
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,("could not construct forwarding socket")); 
	  m_log(M_ERROR, M_QUIET,"could not construct forwarding socket"); 
	}
      } else {
	M_TRACE(MTRACE_ERROR,MTRACE_REPOS,("marshalling error (size=%d)", 
					   sendsize));
	m_log(M_ERROR, M_QUIET,"marshalling error (size=%d)",sendsize);  
      }
      return;
    }
    fwl = fwl->fw_next;
  }
}
