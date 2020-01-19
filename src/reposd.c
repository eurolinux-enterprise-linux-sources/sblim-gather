/*
 * $Id: reposd.c,v 1.33 2009/12/01 23:56:35 tyreld Exp $
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
 * Contributors: Michael Schuele <schuelem@de.ibm.com>
 *
 * Description: Repository Daemon
 *
 * The Performance Data Repository Daemon is receiving and storing 
 * metric information from a Gatherer daemon
 * and returning request results and status information to clients.
 * 
 */

#include "repos.h"
#include "sforward.h"
#include "gatherc.h"
#include "commheap.h"
#include "marshal.h"
#include "reposcfg.h"
#include <mtrace.h>
#include <mlog.h>
#include <mcserv.h>
#include <rcserv.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <memory.h>
#include <pthread.h>

#define CHECKBUFFER(comm,buffer,sz,len) ((comm)->gc_datalen+sizeof(GATHERCOMM)+(sz)<=len)

/* ---------------------------------------------------------------------------*/

static char rreposport_s[10] = {0};
static int  rreposport       = 0;
static char rmaxconn_s[10]   = {0};
static int  rmaxconn         = 0;
static int  connects         = 0;
static pthread_mutex_t connect_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  connect_cond  = PTHREAD_COND_INITIALIZER;

static size_t globalNum = 0;
static int    globalAsc = 0;

static void * repos_remote();
static void * rrepos_getrequest(void * hdl);

#define RINITCHECK() \
pthread_mutex_lock(&connect_mutex); \
if (rreposport==0) { \
  if (reposcfg_getitem("RepositoryPort",rreposport_s,sizeof(rreposport_s)) == 0) {  \
    rreposport=atoi(rreposport_s); \
  } else { \
    rreposport=6363; \
  } \
  if (reposcfg_getitem("RepositoryMaxConnections",rmaxconn_s,sizeof(rmaxconn_s)) == 0) {  \
    rmaxconn=atoi(rmaxconn_s); \
  } else { \
    rmaxconn=100; \
  } \
  rcs_init(&rreposport); \
} \
pthread_mutex_unlock(&connect_mutex); 


/* ---------------------------------------------------------------------------*/

int main(int argc, char * argv[])
{
  int           quit=0;
  MC_REQHDR     hdr;
  GATHERCOMM   *comm;
  COMMHEAP     *ch;
  size_t        buffersize=GATHERVALBUFLEN;
  char         *buffer;
  size_t        bufferlen;
  SubscriptionRequest *sr;
  ValueRequest *vr;
  ValueItem    *vi;
  void         *vp;
  char         *vpmax;
  off_t         vpoffs;
  int           i;
  size_t        valreslen;
  size_t        valsyslen;
  int           numval;
  off_t         offset;
  RepositoryPluginDefinition *rdef;
  char         *pluginname, *metricname, *listener;
  MetricResourceId *rid;
  MetricValue  *mv;
  pthread_t     rcomm;
  char          cfgbuf[1000];
  size_t        filterNum;
  int           filterAsc;
#ifndef NOTRACE
  char         *cfgidx1, *cfgidx2;
#endif

  m_start_logging("reposd");
  m_log(M_INFO,M_QUIET,"Reposd is starting up.\n");

  if (argc == 1) {
    /* daemonize if no arguments are given */
    if (daemon(0,0)) {
      m_log(M_ERROR,M_SHOW,"Couldn't daemonize: %s - exiting\n",
	    strerror(errno));
      exit(-1);
    }
  }
  
  if (reposcfg_init()) {
    m_log(M_ERROR,M_SHOW,"Could not open reposd config file.\n");
  }
  
  /* init tracing from config */
#ifndef NOTRACE
  if (reposcfg_getitem("TraceLevel",cfgbuf,sizeof(cfgbuf))==0) {
    m_trace_setlevel(atoi(cfgbuf));
  }
  if (reposcfg_getitem("TraceFile",cfgbuf,sizeof(cfgbuf)) ==0) {
	m_trace_setfile(cfgbuf);
  }
  if (reposcfg_getitem("TraceComponents",cfgbuf,sizeof(cfgbuf)) ==0) {
    cfgidx1 = cfgbuf;
    while (cfgidx1) {
      cfgidx2 = strchr(cfgidx1,':');
      if (cfgidx2) {
	*cfgidx2++=0;
      }
      m_trace_enable(m_trace_compid(cfgidx1));
      cfgidx1=cfgidx2;
    }
  }
#endif
  
  M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,("Reposd tracing initialized."));

  if (mcs_init(REPOS_COMMID)) {
    m_log(M_ERROR,M_SHOW,"Could not open reposd socket.\n");
    return -1;
  }

  /* start remote reposd in a separate thread */
  RINITCHECK();
  if (pthread_create(&rcomm,NULL,repos_remote,NULL) != 0) {
    m_log(M_ERROR,M_SHOW,"Could not create remote reposd thread: %s.\n",
	  strerror(errno));
    return -1;
  }
  pthread_detach(rcomm);

  /* check whether autoloading is requested */
  if (reposcfg_getitem("AutoLoad",cfgbuf,sizeof(cfgbuf))==0) {
    int              rc;
    M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,("Automatic initialization starting..."));
    rc = repos_init();
    if (rc) {
      m_log(M_ERROR,M_SHOW,"Could not auto-initialize.\n");
      return -1;
    }
  }

  buffer = malloc(buffersize);
  bufferlen = buffersize;
 
  while (!quit && mcs_accept(&hdr)==0) {
    while (!quit && mcs_getrequest(&hdr, buffer, &bufferlen)==0) {
      M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Received message type=%d",hdr.mc_type));
      if (hdr.mc_type != GATHERMC_REQ) {
	/* ignore unknown message types */
	m_log(M_ERROR,M_QUIET,"Invalid request type  received %c\n",hdr.mc_type);
	continue;
      }
      comm=(GATHERCOMM*)buffer;
      /* perform sanity check */
      if (bufferlen != sizeof(GATHERCOMM) + comm->gc_datalen) {
	m_log(M_ERROR,M_QUIET,"Invalid length received: expected %d got %d\n",
	      sizeof(GATHERCOMM)+comm->gc_datalen,bufferlen);
	continue;
      }
      M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Received command id=%d",comm->gc_cmd));
      switch (comm->gc_cmd) {
      case GCMD_STATUS:
	repos_status((RepositoryStatus*)(buffer+sizeof(GATHERCOMM)));
	comm->gc_result=0;
	comm->gc_datalen=sizeof(RepositoryStatus);
	break;
      case GCMD_INIT:
	comm->gc_result=repos_init();
	comm->gc_datalen=0;
	break;
      case GCMD_TERM:
	comm->gc_result=repos_terminate();
	comm->gc_datalen=0;
	break;
      case GCMD_GETTOKEN:
	comm->gc_result=
	  repos_sessiontoken((RepositoryToken*)(buffer+sizeof(GATHERCOMM)));
	comm->gc_datalen=sizeof(RepositoryToken);
	break;
      case GCMD_ADDPLUGIN:
	offset = sizeof(GATHERCOMM);
	if (unmarshal_string(&pluginname,buffer,&offset,buffersize,1) == 0) {
	  comm->gc_result=reposplugin_add(pluginname);
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Unmarshalling add plugin request failed %d/%d.",
		   offset,buffersize));
	  m_log(M_ERROR,M_QUIET,
		"Unmarshalling add plugin request failed %d/%d.",
		offset,buffersize);
	  comm->gc_result=-1;
	}
	if (comm->gc_result != 0) {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Could not load repository plugin %s.",
		   pluginname));
	  m_log(M_ERROR,M_QUIET,
		"Could not load repository plugin %s.",
		pluginname);		
	}
	comm->gc_datalen=0;
	break;
      case GCMD_REMPLUGIN:
	offset = sizeof(GATHERCOMM);
	if (unmarshal_string(&pluginname,buffer,&offset,buffersize,1) == 0) {
	  comm->gc_result=reposplugin_remove(pluginname);
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Unmarshalling remove plugin request failed %d/%d.",
		   offset,buffersize));
	  m_log(M_ERROR,M_QUIET,
		"Unmarshalling remove plugin request failed %d/%d.",
		offset,buffersize);
	  comm->gc_result=-1;
	}
	if (comm->gc_result != 0) {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Could not unload repository plugin %s.",
		   pluginname));
	  m_log(M_ERROR,M_QUIET,
		"Could not unload repository plugin %s.",
		pluginname);		
	}
	comm->gc_datalen=0;
	break;
      case GCMD_LISTPLUGIN:
	offset = sizeof(GATHERCOMM);
	ch=ch_init();
	if (unmarshal_string(&pluginname,buffer,&offset,buffersize,1) == 0) {
	  comm->gc_result=reposplugin_list(pluginname,
					   &rdef,
					   ch);
	  if (comm->gc_result > 0) {
	    /* heuristical size check */
	    if (buffersize < comm->gc_result * (sizeof(RepositoryPluginDefinition) + 40 ) ) {
	      buffersize += comm->gc_result * (sizeof(RepositoryPluginDefinition) + 40 );
	      buffer = realloc(buffer, buffersize);
	      comm = (GATHERCOMM*)buffer;
	    }
	    for (i=0;1;i++) {
	      /* heuristical retry strategy - double the buffer size */
	      if (marshal_reposplugindefinition(rdef,comm->gc_result,buffer,
						&offset,buffersize) == 0) {
		break;
	      }
	      if (i < 3) {
		buffersize = buffersize * 2;
		buffer = realloc(buffer, buffersize);
		comm = (GATHERCOMM*)buffer;
		continue;
	      }
	      M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		      ("Marshalling list plugin response failed %d/%d.",
		       offset,buffersize));
	      m_log(M_ERROR,M_QUIET,
		    "Marshalling list plugin response failed %d/%d.",
		    offset,buffersize);
	      comm->gc_result=-1;
	      break;
	    }
	  }
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Unmarshalling list plugin request failed %d/%d.",
		   offset,buffersize));
	  m_log(M_ERROR,M_QUIET,
		"Unmarshalling list plugin request failed %d/%d.",
		 offset,buffersize);
	  comm->gc_result=-1;
	}
	if (comm->gc_result == -1) {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Could not list repository plugin %s definitions.",
		   pluginname));
	  m_log(M_ERROR,M_QUIET,
		"Could not list repository plugin %s definitions.",
		pluginname);		
	}
	comm->gc_datalen=offset;
	ch_release(ch);
	break;
      case GCMD_LISTRESOURCES:
	ch=ch_init();
	comm->gc_result=reposresource_list(buffer+sizeof(GATHERCOMM),
					   &rid,
					   ch);
	if (comm->gc_result > 0) {
	  if (!CHECKBUFFER(comm,buffer,strlen(buffer+sizeof(GATHERCOMM))+ 1 +
			  comm->gc_result*sizeof(MetricResourceId),buffersize)) {
	    /* heuristic approach for resizing of buffer */
	    buffersize += comm->gc_result * (sizeof(MetricResourceId) + 40 );
	    buffer = realloc(buffer, buffersize);
	    comm = (GATHERCOMM*)buffer;
	  }
	  comm->gc_datalen=strlen(buffer+sizeof(GATHERCOMM))+ 1 +
	    comm->gc_result*sizeof(MetricResourceId);
	  memcpy(buffer+sizeof(GATHERCOMM)+strlen(buffer+sizeof(GATHERCOMM))+1,
		 rid,
		 comm->gc_result*sizeof(MetricResourceId));
	  for (i=0; i < comm->gc_result; i++) {
	    if (!CHECKBUFFER(comm,buffer,strlen(rid[i].mrid_resource) + 1,buffersize)) {
	      /* heuristic approach for resizing of buffer */
	      buffersize = buffersize + (comm->gc_result)*strlen(rid[i].mrid_resource);
	      buffer = realloc(buffer, buffersize);
	      comm = (GATHERCOMM*)buffer;
	    }
	    memcpy(buffer+sizeof(GATHERCOMM)+comm->gc_datalen,
		   rid[i].mrid_resource,
		   strlen(rid[i].mrid_resource) + 1);
	    comm->gc_datalen += strlen(rid[i].mrid_resource) + 1;
	    if (!CHECKBUFFER(comm,buffer,strlen(rid[i].mrid_system) + 1,buffersize)) {
	      /* heuristic approach for resizing of buffer */
	      buffersize = buffersize + (comm->gc_result)*strlen(rid[i].mrid_system);
	      buffer = realloc(buffer, buffersize);
	      comm = (GATHERCOMM*)buffer;
	    }
	    memcpy(buffer+sizeof(GATHERCOMM)+comm->gc_datalen,
		   rid[i].mrid_system,
		   strlen(rid[i].mrid_system) + 1);
	    comm->gc_datalen += strlen(rid[i].mrid_system) + 1;
	  }
	} else {
	  comm->gc_datalen=strlen(buffer+sizeof(GATHERCOMM))+ 1;
	}
	ch_release(ch);
	break;
      case GCMD_SUBSCRIBE:
	offset = sizeof(GATHERCOMM);
	if (unmarshal_string(&listener,buffer,&offset,buffersize,1) == 0 &&
	    unmarshal_subscriptionrequest(&sr,buffer,&offset,buffersize) 
	    == 0) {
	  if (subs_enable_forwarding(sr,listener)==0) {
	    comm->gc_result=repos_subscribe(sr,subs_forward);
	  } else {
	    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		    ("Enabling of susbcription forwarding failed."));
	    comm->gc_result=-1;
	  }
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Unmarshalling subscription request failed %d/%d.",
		   offset,buffersize));
	  comm->gc_result=-1;
	}
	comm->gc_datalen=0;
	break;
      case GCMD_UNSUBSCRIBE:
	offset = sizeof(GATHERCOMM);
	if (unmarshal_string(&listener,buffer,&offset,buffersize,1) == 0 &&
	    unmarshal_subscriptionrequest(&sr,buffer,&offset,buffersize) 
	    == 0) {
	  if (subs_disable_forwarding(sr,listener)==0) {
	    comm->gc_result=repos_unsubscribe(sr,subs_forward);
	  } else {
	    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		    ("Disabling of susbcription forwarding failed."));
	    comm->gc_result=-1;
	  }
	} else {
	  M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
		  ("Unmarshalling unsubscription request failed %d/%d.",
		   offset,buffersize));
	  comm->gc_result=-1;
	}
	comm->gc_datalen=0;
	break;
      case GCMD_SETFILTER:
	offset = sizeof(GATHERCOMM);
	if (unmarshal_fixed(&globalNum,sizeof(size_t),
			    buffer,&offset,buffersize) == 0 &&
	    unmarshal_fixed(&globalAsc,sizeof(int),
			    buffer,&offset,buffersize) == 0) {
	  comm->gc_result=0;
	} else {
	  comm->gc_result=-1;
	}
	comm->gc_datalen=0;
	break;      
      case GCMD_GETFILTER:
	offset = sizeof(GATHERCOMM);
	if (marshal_data(&globalNum,sizeof(size_t),
			 buffer,&offset,buffersize) == 0 &&
	    marshal_data(&globalAsc,sizeof(int),
			 buffer,&offset,buffersize) == 0) {
	  comm->gc_result=0;
	} else {
	  comm->gc_result=-1;
	}
	comm->gc_datalen=offset-sizeof(GATHERCOMM);
	break;      
      case GCMD_SETVALUE:
	/* the transmitted parameters are
	 * 1: pluginname
	 * 2: metricname
	 * 3: metricvalue
	 */
	pluginname=buffer+sizeof(GATHERCOMM);
	metricname=pluginname+strlen(pluginname)+1;
	mv=(MetricValue*)(metricname+strlen(metricname)+1);
	/* fixups */
	if (mv->mvResource) {
	  mv->mvResource=(char*)(mv + 1);
	  mv->mvData=mv->mvResource + strlen(mv->mvResource) + 1;
	} else {
	  mv->mvData=(char*)(mv + 1);
	}
	mv->mvSystemId=mv->mvData+mv->mvDataLength;
	comm->gc_result=reposvalue_put(pluginname,metricname,mv);
	comm->gc_datalen=0;
	break;      
      case GCMD_GETVALUE:
	/* the transmitted ValueRequest contains
	 * 1: Header
	 * 2: ResourceName
	 * 3: System Id
	 * 4: ValueItems
	 */
	vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	vpmax=(char*)buffer+buffersize;
	if (vr->vsResource) {
	  /* adjust pointer to resource name */
	  vr->vsResource = (char*)vr + sizeof(ValueRequest);
	  valreslen= strlen(vr->vsResource) + 1;
	} else {
	  valreslen = 0;
	}
	if (vr->vsSystemId) {
	  /* adjust pointer to system id */
	  vr->vsSystemId = (char*)vr + sizeof(ValueRequest) + valreslen;
	  valsyslen = strlen(vr->vsSystemId) + 1;
	} else {
	  valsyslen = 0;
	}
	vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
	vr->vsValues=NULL;
	ch=ch_init();
	
	/* check if global filter is requested */
	if (globalNum == 0) {
	  comm->gc_result=reposvalue_get(vr,ch);
	} else {
	  comm->gc_result=reposvalue_getfiltered(vr,ch,globalNum, globalAsc);
	} 
	/* copy value data into transfer buffer and compute total length */
	numval = vr->vsNumValues;
	if (comm->gc_result != -1) {
	  if ((char*)vi+sizeof(ValueItem)*numval > vpmax) {
	    /* reallocate buffer to required size and adjust ptrs */
	    buffersize+=sizeof(ValueItem)*numval;
	    buffer = realloc(buffer,buffersize);
	    comm = (GATHERCOMM*)buffer;
	    vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	    vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
	    vpmax = buffer + buffersize;
	  } 
	  memcpy(vi,vr->vsValues,sizeof(ValueItem)*numval);
	  vp = (char*)vi + sizeof(ValueItem) * numval;
	  for (i=0;i<numval;i++) {
	    M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
		    ("Returning value for mid=%d, resource %s: %d",
		     vr->vsId,
		     vr->vsValues[i].viResource,
		     *(int*)vr->vsValues[i].viValue));
	    if ((char*)vp + vr->vsValues[i].viValueLen > vpmax) {
	      /* reallocate buffer to required size and adjust ptrs */
	      /* somewhat heuristic - we assume all the value sizes are equal */
	      vpoffs = vp - (void*)vi;
	      buffersize += vr->vsValues[i].viValueLen*numval;
	      buffer = realloc(buffer,buffersize);
	      comm = (GATHERCOMM*)buffer;
	      vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	      vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
	      vp = (void*)vi + vpoffs;
	      vpmax = buffer + buffersize;
	    }
	    memcpy(vp,vr->vsValues[i].viValue,vr->vsValues[i].viValueLen);
	    vi[i].viValue = vp;
	    vp = (char*)vp + vi[i].viValueLen;
	    if (vr->vsValues[i].viResource) {
	      if ((char*)vp + strlen(vr->vsValues[i].viResource) + 1 > vpmax) {
		/* reallocate buffer to required size and adjust ptrs */
		/* probably allocating to much in the long run */
		vpoffs = vp - (void*)vi;
		buffersize += (strlen(vr->vsValues[i].viResource)+1)*numval;
		buffer = realloc(buffer,buffersize);
		comm = (GATHERCOMM*)buffer;			    
		vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
		vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
		vp = (void*)vi + vpoffs;
		vpmax = buffer + buffersize;
	      }
	      strcpy(vp,vr->vsValues[i].viResource);
	      vi[i].viResource = vp;
	      vp = (char*)vp + strlen(vp)+1;
	    } else {
	      comm->gc_result=-1;
	      break;
	    }
	    if (vr->vsValues[i].viSystemId) {
	      if ((char*)vp + strlen(vr->vsValues[i].viSystemId) + 1 > vpmax) {
		/* reallocate buffer to required size and adjust ptrs */
		/* probably allocating to much in the long run */
		vpoffs = vp - (void*)vi;
		buffersize += (strlen(vr->vsValues[i].viSystemId)+1)*numval;
		buffer = realloc(buffer,buffersize);
		comm = (GATHERCOMM*)buffer;
		vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
		vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
		vp = (void*)vi + vpoffs;
		vpmax = buffer + buffersize;
	      }
	      strcpy(vp,vr->vsValues[i].viSystemId);
	      vi[i].viSystemId = vp;
	      vp = (char*)vp + strlen(vp)+1;
	    } else {
	      comm->gc_result=-1;
	      break;
	    }
	  }
	  comm->gc_datalen= (char*)vp - (char*)vr;
	}
	ch_release(ch);
	break;
      case GCMD_GETFILTERED:
	/* the transmitted ValueRequest contains
	 * 1: Header
	 * 2: ResourceName
	 * 3: System Id
	 * 4: Number of Requested Items
	 * 5: Ascending (1=yes)
	 * 6: ValueItems
	 */
	vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	vpmax=(char*)buffer+buffersize;
	if (vr->vsResource) {
	  /* adjust pointer to resource name */
	  vr->vsResource = (char*)vr + sizeof(ValueRequest);
	  valreslen= strlen(vr->vsResource) + 1;
	} else {
	  valreslen = 0;
	}
	if (vr->vsSystemId) {
	  /* adjust pointer to system id */
	  vr->vsSystemId = (char*)vr + sizeof(ValueRequest) + valreslen;
	  valsyslen = strlen(vr->vsSystemId) + 1;
	} else {
	  valsyslen = 0;
	}
	filterNum=*(size_t*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen);
	filterAsc=*(int*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t));
	vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t)+sizeof(int));
	vr->vsValues=NULL;
	ch=ch_init();
	
	comm->gc_result=reposvalue_getfiltered(vr,ch,filterNum,filterAsc);
	/* copy value data into transfer buffer and compute total length */
	numval = vr->vsNumValues;
	if (comm->gc_result != -1) {
	  if ((char*)vi+sizeof(ValueItem)*numval > vpmax) {
	    /* reallocate buffer to required size and adjust ptrs */
	    buffersize+=sizeof(ValueItem)*numval;
	    buffer = realloc(buffer,buffersize);
	    comm = (GATHERCOMM*)buffer;
	    vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	    vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t)+sizeof(int));
	    vpmax = buffer + buffersize;
	  } 
	  memcpy(vi,vr->vsValues,sizeof(ValueItem)*numval);
	  vp = (char*)vi + sizeof(ValueItem) * numval;
	  for (i=0;i<numval;i++) {
	    M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
		    ("Returning value for mid=%d, resource %s: %d",
		     vr->vsId,
		     vr->vsValues[i].viResource,
		     *(int*)vr->vsValues[i].viValue));
	    if ((char*)vp + vr->vsValues[i].viValueLen > vpmax) {
	      /* reallocate buffer to required size and adjust ptrs */
	      /* somewhat heuristic - we assume all the value sizes are equal */
	      vpoffs = vp - (void*)vi;
	      buffersize += vr->vsValues[i].viValueLen*numval;
	      buffer = realloc(buffer,buffersize);
	      comm = (GATHERCOMM*)buffer;
	      vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
	      vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t)+sizeof(int));
	      vp = (void*)vi + vpoffs;
	      vpmax = buffer + buffersize;
	    }
	    memcpy(vp,vr->vsValues[i].viValue,vr->vsValues[i].viValueLen);
	    vi[i].viValue = vp;
	    vp = (char*)vp + vi[i].viValueLen;
	    if (vr->vsValues[i].viResource) {
	      if ((char*)vp + strlen(vr->vsValues[i].viResource) + 1 > vpmax) {
		/* reallocate buffer to required size and adjust ptrs */
		/* probably allocating to much in the long run */
		vpoffs = vp - (void*)vi;
		buffersize += (strlen(vr->vsValues[i].viResource)+1)*numval;
		buffer = realloc(buffer,buffersize);
		comm = (GATHERCOMM*)buffer;			    
		vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
		vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t)+sizeof(int));
		vp = (void*)vi + vpoffs;
		vpmax = buffer + buffersize;
	      }
	      strcpy(vp,vr->vsValues[i].viResource);
	      vi[i].viResource = vp;
	      vp = (char*)vp + strlen(vp)+1;
	    } else {
	      comm->gc_result=-1;
	      break;
	    }
	    if (vr->vsValues[i].viSystemId) {
	      if ((char*)vp + strlen(vr->vsValues[i].viSystemId) + 1 > vpmax) {
		/* reallocate buffer to required size and adjust ptrs */
		/* probably allocating to much in the long run */
		vpoffs = vp - (void*)vi;
		buffersize += (strlen(vr->vsValues[i].viSystemId)+1)*numval;
		buffer = realloc(buffer,buffersize);
		comm = (GATHERCOMM*)buffer;
		vr=(ValueRequest *)(buffer+sizeof(GATHERCOMM));
		vi=(ValueItem*)((char*)vr+sizeof(ValueRequest)+valreslen+valsyslen+sizeof(size_t)+sizeof(int));
		vp = (void*)vi + vpoffs;
		vpmax = buffer + buffersize;
	      }
	      strcpy(vp,vr->vsValues[i].viSystemId);
	      vi[i].viSystemId = vp;
	      vp = (char*)vp + strlen(vp)+1;
	    } else {
	      comm->gc_result=-1;
	      break;
	    }
	  }
	  comm->gc_datalen= (char*)vp - (char*)vr;
	}
	ch_release(ch);
	break;
      case GCMD_QUIT:
	quit=1;
	/* terminate as well */
	comm->gc_result=repos_terminate();
	comm->gc_datalen=0;
	break;
      default:
	comm->gc_result=-1;
	comm->gc_datalen=0;
	break;
      }
      if (sizeof(GATHERCOMM) + comm->gc_datalen > buffersize) {
	m_log(M_ERROR,M_QUIET,
	      "Error: Available data size is exceeding buffer.\n");
      }
      if (sizeof(GATHERCOMM) + comm->gc_datalen > GATHERVALBUFLEN) {
	/* send out a RESPMORE response */
	struct {
	  GATHERCOMM comm;
	  size_t     sz;
	} moretocomm;
	moretocomm.comm.gc_result = 0;
	moretocomm.comm.gc_datalen = sizeof(size_t);
	moretocomm.sz = sizeof(GATHERCOMM)+comm->gc_datalen;
	hdr.mc_type=GATHERMC_RESPMORE;
	M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Big buffer size: %z, responding with GATHERMC_RESPMORE.",
					  moretocomm.sz));
	mcs_sendresponse(&hdr,&moretocomm,sizeof(moretocomm));
      }
      hdr.mc_type=GATHERMC_RESP;
      mcs_sendresponse(&hdr,buffer,sizeof(GATHERCOMM)+comm->gc_datalen);
      bufferlen=buffersize;
    }
  }
  mcs_term();
  m_log(M_INFO,M_QUIET,"Reposd is shutting down.\n");
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Reposd is shutting down."));
  if (buffer) {
    free(buffer);
  }
  return 0;
}

static void * repos_remote()
{
  pthread_t thread;
  int       hdl = -1;
  
  m_log(M_INFO,M_QUIET,"Remote reposd is starting up.\n");
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Remote reposd is starting up."));

  while (1) {
    if (hdl == -1) {
      if (rcs_accept(&hdl) == -1) {
	m_log(M_ERROR,M_SHOW,"Remote reposd could not accept: %s.\n",
	      strerror(errno));
	return 0; 
      }
      M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Remote client request on socket %i accepted",hdl));
    }
    pthread_mutex_lock(&connect_mutex);
    connects++;
    M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	    ("Increased number of current remote client connections %i",connects));
    pthread_mutex_unlock(&connect_mutex);
    if (pthread_create(&thread,NULL,rrepos_getrequest,(void*)(long)hdl) != 0) {
      m_log(M_ERROR,M_SHOW,"Remote reposd could not create thread for socket %i: %s.\n",
	    hdl,strerror(errno));
      return 0;
    }   
    pthread_mutex_lock(&connect_mutex);
    if(connects==rmaxconn) {
      /* wait for at least one finished thread */
      M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	      ("Remote reposd's MaxConnections reached %i - waiting for at least one finished thread",
	       connects));
      pthread_cond_wait(&connect_cond,&connect_mutex);
    }
    pthread_mutex_unlock(&connect_mutex);
    hdl = -1;
  }
  rcs_term();
  m_log(M_INFO,M_QUIET,"Remote reposd is shutting down.\n");
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Remote reposd is shutting down."));
  return 0;
}

static void * rrepos_getrequest(void * hdl)
{
  GATHERCOMM    *comm;
  char          *buffer = malloc(GATHERVALBUFLEN);
  size_t         bufferlen;
  char          *pluginname, *metricname;
  int            is64=0;
  MetricValue    mvTemp;
  MetricValue   *mv;
  MetricValue64 *mv64;

  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Starting thread on socket %i",(long)hdl));
  if (pthread_detach(pthread_self()) != 0) {
    m_log(M_ERROR,M_SHOW,"Remote reposd thread could not detach: %s.\n",
	  strerror(errno));
  }
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Detached thread on socket %i",(long)hdl));

  while (1) {
    bufferlen=GATHERVALBUFLEN;

    pthread_mutex_lock(&connect_mutex);

    if (rcs_getrequest((long)hdl,buffer,&bufferlen) == -1) {
      connects--;
      M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	    ("Decreased number of current remote client connections %i",connects));
      pthread_cond_signal(&connect_cond);
      pthread_mutex_unlock(&connect_mutex);
      break;
    }
    pthread_mutex_unlock(&connect_mutex);

    /* write data to repository */
    comm=(GATHERCOMM*)buffer;    
    comm->gc_cmd     = ntohs(comm->gc_cmd);
    comm->gc_datalen = ntohl(comm->gc_datalen);
    comm->gc_result  = ntohs(comm->gc_result);
        
    /* perform sanity check */
    if (bufferlen != sizeof(GATHERCOMM) + comm->gc_datalen) {
      m_log(M_ERROR,M_SHOW,
	    "Remote reposd invalid length received on socket %i: expected %d got %d.\n",
	    hdl,sizeof(GATHERCOMM)+comm->gc_datalen,bufferlen);
      continue;
    }
    /* the transmitted parameters are
     * 1: pluginname
     * 2: metricname
     * 3: metricvalue
     */
    pluginname=buffer+sizeof(GATHERCOMM);
    metricname=pluginname+strlen(pluginname)+1;
    mv=(MetricValue*)(metricname+strlen(metricname)+1);
    mv64=(MetricValue64*)mv;

    if (mv64->mv64Filler1 == 0xff0000ff && mv64->mv64Filler2 == 0x00ffff00) {
      is64 = 1;
    }
 
#if SIZEOF_LONG == 8
    if (!is64) {
      MetricValue32 *mv32=(MetricValue32*)mv;
      /* bitness fixups for 32 bit source to 64 bit target */
      /* must move data block "upwards" to create room for 32-bit structure */
      if (bufferlen + sizeof(MetricValue) - sizeof(MetricValue32) > GATHERVALBUFLEN) {
	/* not enough room to expand ! */
	m_log(M_ERROR,M_SHOW,
	      "Remote reposd short buffer on socket %i during 32-to-64-bit expansion.\n",
	      hdl);
	continue;    	
      }
      mvTemp.mvId         = mv32->mv32Id;
      mvTemp.mvTimeStamp  = mv32->mv32TimeStamp;
      mvTemp.mvDataType   = mv32->mv32DataType;
      mvTemp.mvDataLength = mv32->mv32DataLength;
      mvTemp.mvResource   = mv32->mv32Resource ? (char*)(mv+1) : NULL;
      memmove(mv+1,mv32+1,
	      comm->gc_datalen-(((char*)mv-pluginname)+sizeof(MetricValue32)));
      mv->mvId         = mvTemp.mvId;
      mv->mvTimeStamp  = mvTemp.mvTimeStamp;
      mv->mvDataType   = mvTemp.mvDataType;
      mv->mvDataLength = mvTemp.mvDataLength;
      mv->mvResource   = mvTemp.mvResource;
    }
#else
    if (is64) {
      /* mv64=mv is already set */
      /* bitness fixups for 64 bit source to 32 bit target */
      /* must move data "downwards" to make it fit */
      mvTemp.mvId         = mv64->mv64Id;
      mvTemp.mvTimeStamp  = mv64->mv64TimeStamp;
      mvTemp.mvDataType   = mv64->mv64DataType;
      mvTemp.mvDataLength = mv64->mv64DataLength;
      mvTemp.mvResource   = mv64->mv64Resource ? (char*)(mv+1) : NULL;
      memmove(mv+1,mv64+1,
	      comm->gc_datalen-(((char*)mv-pluginname)+sizeof(MetricValue64)));     
      mv->mvId         = mvTemp.mvId;
      mv->mvTimeStamp  = mvTemp.mvTimeStamp;
      mv->mvDataType   = mvTemp.mvDataType;
      mv->mvDataLength = mvTemp.mvDataLength;
      mv->mvResource   = mvTemp.mvResource;
    }
#endif

    /* fixups */
    if (mv->mvResource) {
      mv->mvResource=(char*)(mv + 1);
      mv->mvData=mv->mvResource + strlen(mv->mvResource) + 1;
    } else {
      mv->mvData=(char*)(mv + 1);
    }

    /* convert from host byte order into network byte order */
    mv->mvId         = ntohl((int)mv->mvId);
    mv->mvTimeStamp  = ntohl((time_t)mv->mvTimeStamp);
    mv->mvDataType   = ntohl((unsigned)mv->mvDataType);
    mv->mvDataLength = ntohl((size_t)mv->mvDataLength);
    mv->mvSystemId=mv->mvData+mv->mvDataLength;
    M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	    ("Retrieved data on socket %i: %s %s %s",(long)hdl,
	     mv->mvSystemId,pluginname,metricname));
    if ((comm->gc_result=reposvalue_put(pluginname,metricname,mv)) != 0) {
      m_log(M_ERROR,M_SHOW,"Remote reposd on socket %i: write %s to repository failed.\n",
	    hdl,metricname);
    }
  }
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("Ending thread on socket %i",(long)hdl));
  free(buffer);
  return NULL;
}
