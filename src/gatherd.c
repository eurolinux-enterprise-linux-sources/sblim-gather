/*
 * $Id: gatherd.c,v 1.14 2009/06/19 20:04:54 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2004, 2009
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
 * Description: Gatherer Daemon
 *
 * The Performance Data Gatherer Daemon is processing requests from
 * a client and returning request results and status information.
 * 
 */

#include "gather.h"
#include "gatherc.h"
#include <commheap.h>
#include <gathercfg.h>
#include <mtrace.h>
#include <mlog.h>
#include <mcserv.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define CHECKBUFFER(comm,buffer,sz,len) ((comm)->gc_datalen+sizeof(GATHERCOMM)+(sz)<=len)

int main(int argc, char * argv[])
{
  int           quit=0;
  MC_REQHDR     hdr;
  GATHERCOMM   *comm;
  COMMHEAP     *ch;
  size_t        buffersize=GATHERVALBUFLEN;
  char         *buffer;
  size_t        bufferlen;
  char          cfgbuf[1000];
#ifndef NOTRACE
  char         *cfgidx1, *cfgidx2;
#endif
  int           i;
  PluginDefinition *pdef;

  m_start_logging("gatherd");
  m_log(M_INFO,M_QUIET,"Gatherd is starting up.\n");
  
  if (argc == 1) {
    /* daemonize if no arguments are given */
    if (daemon(0,0)) {
      m_log(M_ERROR,M_SHOW,"Couldn't daemonize: %s - exiting\n",
	    strerror(errno));
      exit(-1);
    }
  }

  if (gathercfg_init()) {
    m_log(M_ERROR,M_SHOW,"Could not open gatherd config file.\n");
  }
  
  /* init tracing from config */
#ifndef NOTRACE
  if (gathercfg_getitem("TraceLevel",cfgbuf,sizeof(cfgbuf))==0) {
    m_trace_setlevel(atoi(cfgbuf));
  }
  if (gathercfg_getitem("TraceFile",cfgbuf,sizeof(cfgbuf)) ==0) {
	m_trace_setfile(cfgbuf);
  }
  if (gathercfg_getitem("TraceComponents",cfgbuf,sizeof(cfgbuf)) ==0) {
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
  
  M_TRACE(MTRACE_DETAILED,MTRACE_GATHER,("Gatherd tracing initialized."));

  if (mcs_init(GATHER_COMMID)) {
    m_log(M_ERROR,M_SHOW,"Could not open gatherd socket.\n");
    return -1;
  }

  /* check whether autoloading is requested */
  if (gathercfg_getitem("AutoLoad",cfgbuf,sizeof(cfgbuf))==0) {
    int          rc;
    GatherStatus gs;
    M_TRACE(MTRACE_DETAILED,MTRACE_GATHER,("Automatic initialization starting..."));
    rc = gather_init();
    if (rc) {
      m_log(M_ERROR,M_SHOW,"Could not auto-initialize.\n");
      return -1;
    }
    gather_status(&gs);
    if (gs.gsInitialized) {
      if (gather_start()) {
	m_log(M_ERROR,M_SHOW,"Could not auto-start.\n");
	return -1;
      }
    }
  }

  buffer=malloc(buffersize);
  bufferlen=buffersize;

  while (!quit && mcs_accept(&hdr)==0) {
    while (!quit && mcs_getrequest(&hdr, buffer, &bufferlen)==0) {
      M_TRACE(MTRACE_FLOW,MTRACE_GATHER,("Received message type=%d",hdr.mc_type));
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
      M_TRACE(MTRACE_FLOW,MTRACE_GATHER,("Received command id=%d",comm->gc_cmd));
      switch (comm->gc_cmd) {
      case GCMD_STATUS:
	gather_status((GatherStatus*)(buffer+sizeof(GATHERCOMM)));
	comm->gc_result=0;
	comm->gc_datalen=sizeof(GatherStatus);
	break;
      case GCMD_INIT:
	comm->gc_result=gather_init();
	comm->gc_datalen=0;
	break;
      case GCMD_TERM:
	comm->gc_result=gather_terminate();
	comm->gc_datalen=0;
	break;
      case GCMD_START:
	comm->gc_result=gather_start();
	comm->gc_datalen=0;
	break;
      case GCMD_STOP:
	comm->gc_result=gather_stop();
	comm->gc_datalen=0;
	break;
      case GCMD_ADDPLUGIN:
	comm->gc_result=metricplugin_add(buffer+sizeof(GATHERCOMM));
	comm->gc_datalen=0;
	break;
      case GCMD_REMPLUGIN:
	comm->gc_result=metricplugin_remove(buffer+sizeof(GATHERCOMM));
	comm->gc_datalen=0;
	break;
      case GCMD_LISTPLUGIN:
	ch=ch_init();
	comm->gc_result=metricplugin_list(buffer+sizeof(GATHERCOMM),
					  &pdef,
					  ch);
	if (comm->gc_result > 0) {
	  while (!CHECKBUFFER(comm,buffer,strlen(buffer+sizeof(GATHERCOMM))+ 1 +
			      comm->gc_result*sizeof(PluginDefinition),buffersize)) {
	    /* heuristic approach for resize */
	    buffersize += comm->gc_result * (sizeof(PluginDefinition) + 20 );
	    buffer = realloc(buffer, buffersize);
	    comm = (GATHERCOMM*)buffer;	    
	  }
	  comm->gc_datalen=strlen(buffer+sizeof(GATHERCOMM))+ 1 +
	    comm->gc_result*sizeof(PluginDefinition);
	  memcpy(buffer+sizeof(GATHERCOMM)+strlen(buffer+sizeof(GATHERCOMM))+1,
		 pdef,
		 comm->gc_result*sizeof(PluginDefinition));
	  for (i=0; i < comm->gc_result; i++) {
	    if (!CHECKBUFFER(comm,buffer,strlen(pdef[i].pdName) + 1,buffersize)) {
	      /* heuristic approach for resize */
	      buffersize += strlen(pdef[i].pdName) + 1;
	      buffer = realloc(buffer, buffersize);
	      comm = (GATHERCOMM*)buffer;	    
	    }
	    memcpy(buffer+sizeof(GATHERCOMM)+comm->gc_datalen,
		   pdef[i].pdName,
		   strlen(pdef[i].pdName) + 1);
	    comm->gc_datalen += strlen(pdef[i].pdName) + 1;
	  }  
	} else {
	  comm->gc_datalen=strlen(buffer+sizeof(GATHERCOMM))+ 1;
	}
	ch_release(ch);
	break;
      case GCMD_QUIT:
	quit=1;
	comm->gc_result=gather_terminate();;
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
  free(buffer);
  m_log(M_INFO,M_QUIET,"Gatherd is shutting down.\n");
  M_TRACE(MTRACE_FLOW,MTRACE_GATHER,("Gatherd is shutting down.\n"));
  return 0;
}
