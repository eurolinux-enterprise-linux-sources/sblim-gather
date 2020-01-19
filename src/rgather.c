/*
 * $Id: rgather.c,v 1.13 2009/05/20 19:39:55 tyreld Exp $
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
 * Description: Remote Gatherer Access Library
 * Implementation of the Remote API use to control the Gatherer. 
 * 
 */

#include "rgather.h"
#include "gatherc.h"
#include "mcclt.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

static int rgatherhandle=-1;

static pthread_mutex_t rgather_mutex = PTHREAD_MUTEX_INITIALIZER;

#define INITCHECK() \
pthread_mutex_lock(&rgather_mutex); \
if (rgatherhandle==-1) { \
  rgatherhandle=mcc_init(GATHER_COMMID); \
} \
pthread_mutex_unlock(&rgather_mutex);

int rgather_load()
{
  int pid;
  if (system("ps -C gatherd")) {
    /* No gatherd around */
    /*signal(SIGCHLD,SIG_IGN);*/
    pid=fork();
    switch(pid) {
    case 0:
      execlp("gatherd","gatherd",NULL);
      exit(-1);
      break;
    case -1:
      return -1;
      break;
    default:
      waitpid(pid,NULL,0);
      sleep(1); /* todo: need to make sure daemon is initialized */
      break;
    }
  }
  return 0;
}

int rgather_unload()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_QUIT;
  comm->gc_datalen=0;
  comm->gc_result=0;
  pthread_mutex_lock(&rgather_mutex);
  if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rgather_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rgather_mutex);
    return -1;
  }
}

int rgather_init()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_INIT;
  comm->gc_datalen=0;
  comm->gc_result=0;
  pthread_mutex_lock(&rgather_mutex);
  if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rgather_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rgather_mutex);
    return -1;
  }
}

int rgather_terminate()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_TERM;
  comm->gc_datalen=0;
  comm->gc_result=0;
  pthread_mutex_lock(&rgather_mutex);
  if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0 &&
      mcc_term(rgatherhandle)==0) {
    rgatherhandle=-1;
    pthread_mutex_unlock(&rgather_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rgather_mutex);
    return -1;
  }
}

int rgather_start()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_START;
  comm->gc_datalen=0;
  comm->gc_result=0;
  pthread_mutex_lock(&rgather_mutex);
  if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rgather_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rgather_mutex);
    return -1;
  }
}

int rgather_stop()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_STOP;
  comm->gc_datalen=0;
  comm->gc_result=0;
  pthread_mutex_lock(&rgather_mutex);
  if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rgather_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rgather_mutex);
    return -1;
  }
}

int rgather_status(GatherStatus *gs)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  GatherStatus *gsp=(GatherStatus*)(xbuf+sizeof(GATHERCOMM));
  size_t        commlen=sizeof(xbuf);

  if (gs) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_STATUS;
    comm->gc_datalen=0;
    comm->gc_result=0;
    pthread_mutex_lock(&rgather_mutex);
    if (mcc_request(rgatherhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
	mcc_response(&hdr,comm,&commlen)==0 &&
	commlen == sizeof(GatherStatus) + sizeof(GATHERCOMM)) {
      *gs = *gsp;
      pthread_mutex_unlock(&rgather_mutex);
      return comm->gc_result;
    } 
    pthread_mutex_unlock(&rgather_mutex);      
  }    
  return -1;
}

int rmetricplugin_add(const char *pluginname)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);
  
  if (pluginname && *pluginname) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_ADDPLUGIN;
    comm->gc_datalen=strlen(pluginname)+1;
    comm->gc_result=0;
    memcpy(xbuf+sizeof(GATHERCOMM),pluginname,comm->gc_datalen);
    pthread_mutex_lock(&rgather_mutex);
    if (mcc_request(rgatherhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rgather_mutex);
      return comm->gc_result;
    } 
    pthread_mutex_unlock(&rgather_mutex);
  }
  return -1;
}

int rmetricplugin_remove(const char *pluginname)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);

  if (pluginname && *pluginname) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_REMPLUGIN;
    comm->gc_datalen=strlen(pluginname)+1;
    comm->gc_result=0;
    memcpy(xbuf+sizeof(GATHERCOMM),pluginname,comm->gc_datalen);
    pthread_mutex_lock(&rgather_mutex);
    if (mcc_request(rgatherhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rgather_mutex);
      return comm->gc_result;
    }
    pthread_mutex_unlock(&rgather_mutex);
  }        
  return -1;  
}

int rmetricplugin_list(const char *pluginname, PluginDefinition **pdef, 
		       COMMHEAP ch)
{
  MC_REQHDR     hdr;
  char         *xbuf=ch_alloc(ch,GATHERVALBUFLEN);
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=GATHERVALBUFLEN;
  int           i;
  char         *stringpool;

  if (pluginname && *pluginname && pdef) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_LISTPLUGIN;
    comm->gc_datalen=strlen(pluginname)+1;
    comm->gc_result=0;
    memcpy(xbuf+sizeof(GATHERCOMM),pluginname,comm->gc_datalen);
    pthread_mutex_lock(&rgather_mutex);
    if (mcc_request(rgatherhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0 &&
	commlen == (sizeof(GATHERCOMM) + comm->gc_datalen)) {
      if (hdr.mc_type == GATHERMC_RESPMORE) {
	/* allocate buffer */
	commlen = *(size_t*)(comm+1);
	if (commlen > GATHERVALBUFLEN) {
	  xbuf = ch_alloc(ch,commlen);
	  comm = (GATHERCOMM*)xbuf;
	}
	if (mcc_response(&hdr,comm,&commlen) != 0 ||
	    commlen != (sizeof(GATHERCOMM) + comm->gc_datalen)) {
	  pthread_mutex_unlock(&rgather_mutex);
	  return -1;
	}
      }
      /* copy data into result buffer and adjust string pointers */
      if (comm->gc_result>0) {
	*pdef = ch_alloc(ch,comm->gc_result*sizeof(PluginDefinition));
	memcpy(*pdef,xbuf+sizeof(GATHERCOMM)+strlen(pluginname)+1,
	       sizeof(PluginDefinition)*comm->gc_result);
	stringpool=ch_alloc(ch,
			    comm->gc_datalen - 
			    sizeof(GATHERCOMM)+strlen(pluginname)+1+
			    comm->gc_result*sizeof(PluginDefinition));
	memcpy(stringpool,xbuf+sizeof(GATHERCOMM)+strlen(pluginname)+1+
	       comm->gc_result*sizeof(PluginDefinition),
	       comm->gc_datalen - sizeof(GATHERCOMM)+strlen(pluginname)+1+
	       comm->gc_result*sizeof(PluginDefinition));
	for (i=0;i<comm->gc_result;i++) {
	  /* extract plugin name and resource names from string pool */
	  (*pdef)[i].pdName = stringpool;
	  stringpool += strlen(stringpool) + 1;
	}
      }
      pthread_mutex_unlock(&rgather_mutex);
      return comm->gc_result;
    }
    pthread_mutex_unlock(&rgather_mutex);
  }        
  return -1;  
  
}
