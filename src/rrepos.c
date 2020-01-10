/*
 * $Id: rrepos.c,v 1.29 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Remote Repository Access Library
 * Implementation of the Remote API use to control the Repository
 * 
 */

#include "rrepos.h"
#include "gatherc.h"
#include "marshal.h"
#include "slisten.h"
#include "mcclt.h"
#include "mtrace.h"
#include "merrno.h"
#include "rcclt.h"
#include "gathercfg.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>

static RepositoryToken _RemoteToken = {sizeof(RepositoryToken),0,0};
static int rreposhandle=-1;
static char _systemId[265]   = {0};
static char rsystemId[265]   = {0};
static char rreposport_s[10] = {0};
static int  rreposport       = 0;

/* use mutex for I/O serialisation */
static pthread_mutex_t rrepos_mutex = PTHREAD_MUTEX_INITIALIZER;

#define INITCHECK() \
pthread_mutex_lock(&rrepos_mutex); \
if (rreposhandle==-1) { \
  rreposhandle=mcc_init(REPOS_COMMID); \
} \
pthread_mutex_unlock(&rrepos_mutex); 

#define RINITCHECK() \
pthread_mutex_lock(&rrepos_mutex); \
if (strlen(rsystemId)==0) { \
  if (gathercfg_getitem("RepositoryHost",rsystemId,sizeof(rsystemId))) { \
    sprintf(rsystemId,"localhost"); \
  } \
  if (gathercfg_getitem("RepositoryPort",rreposport_s,sizeof(rreposport_s)) == 0) {  \
    rreposport=atoi(rreposport_s); \
  } else { \
    rreposport=6363; \
  } \
  rcc_init(rsystemId,&rreposport); \
} \
pthread_mutex_unlock(&rrepos_mutex); 

#ifdef NAGNAG
typedef struct _IdMap {
  int id_local;
  int id_remote;
} IdMap;

typedef struct _PluginMap {
  char      *pm_name;
  IdMap     *pm_map;
  PluginMap *pm_next;
} PluginMap;

PluginMap *pluginmap = NULL;

static PluginMap *pm_locate(const char *);
static PluginMap *pm_insert(const char *);
static int pm_delete(const char *);
static int pm_map(PluginMap *pm, int local, int remote);
#endif

int rrepos_sessioncheck()
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);
  RepositoryToken *rt=(RepositoryToken*)(xbuf+sizeof(GATHERCOMM));

  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_GETTOKEN;
  comm->gc_datalen=0;
  comm->gc_result=0;
  if (mcc_request(rreposhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0 &&
      commlen == sizeof(RepositoryToken) + sizeof(GATHERCOMM)) {
    if (_RemoteToken.rt_size != ntohl(rt->rt_size)) {
      return -1; /* size mismatch */
    } else if (_RemoteToken.rt1 != rt->rt1 ||
	       _RemoteToken.rt2 != rt->rt2) {
      _RemoteToken.rt1 = rt->rt1;
      _RemoteToken.rt2 = rt->rt2;
      return 1;
    }
    return comm->gc_result;
  } else {
    return -1;
  }
  return 0;
}

int rrepos_register(const PluginDefinition *pdef)
{
  int rc = -1;
#ifdef NAGNAG
  if (pdef) {
    RepositoryPluginDefinition *rdef;
    PluginMap *pm;
    COMMHEAP ch=ch_init();
    if (rreposplugin_list(pdef->pdname,&rdef,ch) == 0) {
      pm_delete(pdef->pdname);
      pm=pm_insert(pdef->pdname);
      while (pdef->
      rc = 0;
    }    
    ch_term(ch);
  }
#endif
  return rc;
}

int rrepos_put(const char *reposplugin, const char *metric, MetricValue *mv)
{
  char              xbuf[GATHERBUFLEN];
  GATHERCOMM       *comm=(GATHERCOMM*)xbuf;
  size_t            dataoffs=sizeof(GATHERCOMM);
  size_t            repospluginlen;
  size_t            metriclen;
  size_t            resourcelen;
  size_t            systemlen;

  if (mv) {
    /* determine system id */
    if (strlen(_systemId)==0) {
      gethostname(_systemId,sizeof(_systemId));
    }
    systemlen=strlen(_systemId) + 1;
    RINITCHECK();
#if SIZEOF_LONG == 8
    /* flag the padding areas */
    mv-> mv64Pad1 = 0xff0000ff;
    mv-> mv64Pad2 = 0x00ffff00;
#endif
    comm->gc_cmd     = htons(GCMD_SETVALUE);
    repospluginlen   = strlen(reposplugin) + 1;
    metriclen        = strlen(metric) + 1;
    resourcelen      = (mv->mvResource?strlen(mv->mvResource) + 1:0);
    comm->gc_datalen = htonl(repospluginlen +
			     metriclen +
			     sizeof(MetricValue) + 
			     resourcelen +
			     mv->mvDataLength +
			     systemlen);
    comm->gc_result  = 0;
    /* prepare data */
    mv->mvId         = htonl(mv->mvId);
    mv->mvTimeStamp  = htonl(mv->mvTimeStamp);
    mv->mvDataType   = htonl(mv->mvDataType);
    mv->mvDataLength = htonl(mv->mvDataLength);
    /* copy data into xbuf */ 
    memcpy(xbuf+dataoffs,reposplugin,repospluginlen);
    dataoffs+=repospluginlen;
    memcpy(xbuf+dataoffs,metric,metriclen);
    dataoffs+=metriclen;
    memcpy(xbuf+dataoffs,mv,sizeof(MetricValue));
    dataoffs+=sizeof(MetricValue);
    if (resourcelen) {
      memcpy(xbuf+dataoffs,mv->mvResource,resourcelen);
      dataoffs+=resourcelen;
    }
    memcpy(xbuf+dataoffs,mv->mvData,ntohl(mv->mvDataLength));
    dataoffs+=ntohl(mv->mvDataLength);
    memcpy(xbuf+dataoffs,_systemId,systemlen);
    pthread_mutex_lock(&rrepos_mutex);
    if (rcc_request(comm,sizeof(GATHERCOMM)+ntohl(comm->gc_datalen)) == 0) {
      pthread_mutex_unlock(&rrepos_mutex);
      return 0;
    }
    pthread_mutex_unlock(&rrepos_mutex);
  }
  return -1;
}

int rrepos_get(ValueRequest *vr, COMMHEAP ch)
{
  MC_REQHDR     hdr;
  char         *xbuf=ch_alloc(ch,GATHERVALBUFLEN);
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=GATHERVALBUFLEN;
  size_t        resourcelen;
  size_t        systemlen;
  void         *vp;
  int           i;

  if (vr) {
    INITCHECK();
    resourcelen = vr->vsResource ? strlen(vr->vsResource) + 1 : 0;
    systemlen = vr->vsSystemId ? strlen(vr->vsSystemId) + 1 : 0;
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_GETVALUE;
    comm->gc_datalen=sizeof(ValueRequest) + resourcelen + systemlen;
    comm->gc_result=0;
    /* copy parameters into xmit buffer and perform fixup for string */
    memcpy(xbuf+sizeof(GATHERCOMM),vr,sizeof(ValueRequest));
    if (resourcelen) {
      /* empty resource is allowed */
      strcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest),vr->vsResource);
    }
    if (systemlen) {
      /* empty system id is allowed */
      strcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest) + resourcelen,
	     vr->vsSystemId);
    }
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
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
	  pthread_mutex_unlock(&rrepos_mutex);
	  return -1;
	}
      }
      if (comm->gc_result==0) {
	/* copy received ValueRequest into callers buffer 
	 * allocate array elements and adjust pointers */
	memcpy(vr,xbuf+sizeof(GATHERCOMM),sizeof(ValueRequest));
	vr->vsValues=ch_alloc(ch,vr->vsNumValues*sizeof(ValueItem));
	/* must consider resource name and system id length */
	vp = xbuf + sizeof(GATHERCOMM) + sizeof(ValueRequest) + 
	  resourcelen + systemlen;
	memcpy(vr->vsValues,vp,sizeof(ValueItem)*vr->vsNumValues);
	vp = (char*)vp + vr->vsNumValues*sizeof(ValueItem);
	for (i=0; i<vr->vsNumValues; i++) {
	  vr->vsValues[i].viValue=ch_alloc(ch,vr->vsValues[i].viValueLen);
	  memcpy(vr->vsValues[i].viValue,vp,vr->vsValues[i].viValueLen);
	  vp = (char*)vp + vr->vsValues[i].viValueLen;  
	  if (vr->vsValues[i].viResource) {
	    vr->vsValues[i].viResource = ch_alloc(ch,strlen(vp)+1);
	    strcpy(vr->vsValues[i].viResource,vp);
	    vp = (char*)vp + strlen(vp) + 1;
	  }
	  if (vr->vsValues[i].viSystemId) {
	    vr->vsValues[i].viSystemId = ch_alloc(ch,strlen(vp)+1);
	    strcpy(vr->vsValues[i].viSystemId,vp);
	    vp = (char*)vp + strlen(vp) + 1;
	  }
	}
	pthread_mutex_unlock(&rrepos_mutex);
	return comm->gc_result;
      }
    }
    pthread_mutex_unlock(&rrepos_mutex);    
  }        
  return -1;  
}

int rrepos_getfiltered(ValueRequest *vr, COMMHEAP ch, size_t num, int ascending)
{
  MC_REQHDR     hdr;
  char         *xbuf=ch_alloc(ch,GATHERVALBUFLEN);
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=GATHERVALBUFLEN;
  size_t        resourcelen;
  size_t        systemlen;
  void         *vp;
  int           i;

  if (vr) {
    INITCHECK();
    resourcelen = vr->vsResource ? strlen(vr->vsResource) + 1 : 0;
    systemlen = vr->vsSystemId ? strlen(vr->vsSystemId) + 1 : 0;
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_GETFILTERED;
    comm->gc_datalen=sizeof(ValueRequest) + resourcelen + systemlen + sizeof(size_t) + sizeof(int);
    comm->gc_result=0;
    /* copy parameters into xmit buffer and perform fixup for string */
    memcpy(xbuf+sizeof(GATHERCOMM),vr,sizeof(ValueRequest));
    if (resourcelen) {
      /* empty resource is allowed */
      strcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest),vr->vsResource);
    }
    if (systemlen) {
      /* empty system id is allowed */
      strcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest) + resourcelen,
	     vr->vsSystemId);
    }
    /* copy num and ascending flag */
    memcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest) + resourcelen + systemlen,
	   &num, sizeof(size_t));
    memcpy(xbuf+sizeof(GATHERCOMM) + sizeof(ValueRequest) + resourcelen + systemlen + sizeof(size_t),
	   &ascending, sizeof(int));
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
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
	  pthread_mutex_unlock(&rrepos_mutex);
	  return -1;
	}
      }
      if (comm->gc_result==0) {
	/* copy received ValueRequest into callers buffer 
	 * allocate array elements and adjust pointers */
	memcpy(vr,xbuf+sizeof(GATHERCOMM),sizeof(ValueRequest));
	vr->vsValues=ch_alloc(ch,vr->vsNumValues*sizeof(ValueItem));
	/* must consider resource name and system id length */
	vp = xbuf + sizeof(GATHERCOMM) + sizeof(ValueRequest) + 
	  resourcelen + systemlen + sizeof(size_t) + sizeof(int);
	memcpy(vr->vsValues,vp,sizeof(ValueItem)*vr->vsNumValues);
	vp = (char*)vp + vr->vsNumValues*sizeof(ValueItem);
	for (i=0; i<vr->vsNumValues; i++) {
	  vr->vsValues[i].viValue=ch_alloc(ch,vr->vsValues[i].viValueLen);
	  memcpy(vr->vsValues[i].viValue,vp,vr->vsValues[i].viValueLen);
	  vp = (char*)vp + vr->vsValues[i].viValueLen;  
	  if (vr->vsValues[i].viResource) {
	    vr->vsValues[i].viResource = ch_alloc(ch,strlen(vp)+1);
	    strcpy(vr->vsValues[i].viResource,vp);
	    vp = (char*)vp + strlen(vp) + 1;
	  }
	  if (vr->vsValues[i].viSystemId) {
	    vr->vsValues[i].viSystemId = ch_alloc(ch,strlen(vp)+1);
	    strcpy(vr->vsValues[i].viSystemId,vp);
	    vp = (char*)vp + strlen(vp) + 1;
	  }
	}
	pthread_mutex_unlock(&rrepos_mutex);
	return comm->gc_result;
      }
    }
    pthread_mutex_unlock(&rrepos_mutex);    
  }        
  return -1;  
}

int rrepos_init()
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
  pthread_mutex_lock(&rrepos_mutex);
  if (mcc_request(rreposhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rrepos_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rrepos_mutex);
    return -1;
  }
}

int rrepos_terminate()
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
  pthread_mutex_lock(&rrepos_mutex);
  if (mcc_request(rreposhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0 &&
      mcc_term(rreposhandle)==0) {
    pthread_mutex_unlock(&rrepos_mutex);
    rreposhandle=-1;
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rrepos_mutex);
    return -1;
  }
}

int rrepos_status(RepositoryStatus *rs)
{
  MC_REQHDR         hdr;
  char              xbuf[GATHERBUFLEN];
  GATHERCOMM       *comm=(GATHERCOMM*)xbuf;
  RepositoryStatus *rsp=(RepositoryStatus*)(xbuf+sizeof(GATHERCOMM));
  size_t            commlen=sizeof(xbuf);

  if (rs) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_STATUS;
    comm->gc_datalen=0;
    comm->gc_result=0;
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
	mcc_response(&hdr,comm,&commlen)==0 &&
	commlen == sizeof(RepositoryStatus) + sizeof(GATHERCOMM)) {
      *rs = *rsp;
      pthread_mutex_unlock(&rrepos_mutex);
      return comm->gc_result;
    } 
    pthread_mutex_unlock(&rrepos_mutex);
  }    
  return -1;
}

int rrepos_setglobalfilter(size_t num, int asc)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);
  off_t         offset=sizeof(GATHERCOMM);
  
  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  if (marshal_data(&num,sizeof(size_t),xbuf,&offset,sizeof(xbuf)) == 0 &&
      marshal_data(&asc,sizeof(int),xbuf,&offset,sizeof(xbuf)) == 0) {
    comm->gc_cmd=GCMD_SETFILTER;
    comm->gc_datalen=offset;
    comm->gc_result=0;
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rrepos_mutex);
      return comm->gc_result;
    } 
    pthread_mutex_unlock(&rrepos_mutex);
  } else {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("rrepos_setglobalfilter marshalling error, %d/%d",
	     offset,sizeof(xbuf)));    
  }
  return -1;
}   

int rrepos_getglobalfilter(size_t *num, int *asc)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);
  off_t         offset=sizeof(GATHERCOMM);
  
  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  if (num && asc) {
    comm->gc_cmd=GCMD_GETFILTER;
    comm->gc_datalen=offset;
    comm->gc_result=0;
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rrepos_mutex);
      if (comm->gc_result == 0) {
	if (unmarshal_fixed(num,sizeof(size_t),
			    xbuf,&offset,sizeof(xbuf)) || 
	    unmarshal_fixed(asc,sizeof(int),
			   xbuf,&offset,sizeof(xbuf)) ) {
	  M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		  ("rrepos_getglobalfilter result unmarshalling error, %d/%d",
		   offset,sizeof(xbuf)));    
	  return -1;
	}
      }
      return comm->gc_result;
    } 
    pthread_mutex_unlock(&rrepos_mutex);
  } else {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("rrepos_getglobalfilter null arguments"));    
  }
  return -1;
}   

int rreposplugin_add(const char *pluginname)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);
  off_t         offset=sizeof(GATHERCOMM);
  
  if (pluginname && *pluginname) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    if (marshal_string(pluginname,xbuf,&offset,sizeof(xbuf),1) == 0) {
      comm->gc_cmd=GCMD_ADDPLUGIN;
      comm->gc_datalen=offset;
      comm->gc_result=0;
      memcpy(xbuf+sizeof(GATHERCOMM),pluginname,comm->gc_datalen);
      pthread_mutex_lock(&rrepos_mutex);
      if (mcc_request(rreposhandle,&hdr,comm,
		      sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	  mcc_response(&hdr,comm,&commlen)==0) {
	pthread_mutex_unlock(&rrepos_mutex);
	return comm->gc_result;
      } 
      pthread_mutex_unlock(&rrepos_mutex);
    } else {
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("rreposplugin_add marshalling error, %d/%d",
	       offset,sizeof(xbuf)));    
    }
  }
  return -1;
}

int rreposplugin_remove(const char *pluginname)
{
  MC_REQHDR     hdr;
  char          xbuf[GATHERBUFLEN];
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=sizeof(xbuf);
  off_t         offset=sizeof(GATHERCOMM);

  if (pluginname && *pluginname) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    if (marshal_string(pluginname,xbuf,&offset,sizeof(xbuf),1) == 0) {
      comm->gc_cmd=GCMD_REMPLUGIN;
      comm->gc_datalen=offset;
      comm->gc_result=0;
      memcpy(xbuf+sizeof(GATHERCOMM),pluginname,comm->gc_datalen);
      pthread_mutex_lock(&rrepos_mutex);
      if (mcc_request(rreposhandle,&hdr,comm,
		      sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	  mcc_response(&hdr,comm,&commlen)==0) {
	pthread_mutex_unlock(&rrepos_mutex);
	return comm->gc_result;
      } 
      pthread_mutex_unlock(&rrepos_mutex);
    } else {
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("rreposplugin_add marshalling error, %d/%d",
	       offset,sizeof(xbuf)));    
    }
  }
  return -1;  
}

int rreposplugin_list(const char *pluginname, 
		      RepositoryPluginDefinition **rdef, 
		      COMMHEAP ch)
{
  MC_REQHDR     hdr;
  char         *xbuf=ch_alloc(ch,GATHERVALBUFLEN);
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=GATHERVALBUFLEN;
  off_t         offset=sizeof(GATHERCOMM);

  if (pluginname && *pluginname) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    if (marshal_string(pluginname,xbuf,&offset,commlen,1) == 0) {
      comm->gc_cmd=GCMD_LISTPLUGIN;
      comm->gc_datalen=offset;
      comm->gc_result=0;
      memcpy(xbuf+sizeof(GATHERCOMM),pluginname,offset-sizeof(GATHERCOMM));
      pthread_mutex_lock(&rrepos_mutex);
      if (mcc_request(rreposhandle,&hdr,comm,
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
	    pthread_mutex_unlock(&rrepos_mutex);
	    return -1;
	  }
	}
	pthread_mutex_unlock(&rrepos_mutex);
	if (comm->gc_result > 0) {
	  if (unmarshal_reposplugindefinition(rdef,comm->gc_result,xbuf,
					      &offset,commlen)) {
	    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
		    ("rreposplugin_list result unmarshalling error, %d/%d",
		     offset,commlen));    
	    return -1;
	  }
	} 
	return comm->gc_result;
      } else {
	pthread_mutex_unlock(&rrepos_mutex);
      }
    } else {
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("rreposplugin_list marshalling error, %d/%d",
	       offset,commlen));    
    }
  }
  return -1;   
}

int rreposresource_list(const char * metricid,
			MetricResourceId **rid, 
			COMMHEAP ch)
{
  MC_REQHDR     hdr;
  char          *xbuf=ch_alloc(ch,GATHERVALBUFLEN);
  GATHERCOMM   *comm=(GATHERCOMM*)xbuf;
  size_t        commlen=GATHERVALBUFLEN;
  int           i;
  char         *stringpool;

  if (atoi(metricid)!=-1 && rid) {
    INITCHECK();
    hdr.mc_type=GATHERMC_REQ;
    hdr.mc_handle=-1;
    comm->gc_cmd=GCMD_LISTRESOURCES;
    comm->gc_datalen=strlen(metricid)+1;
    comm->gc_result=0;
    memcpy(xbuf+sizeof(GATHERCOMM),metricid,comm->gc_datalen);
    pthread_mutex_lock(&rrepos_mutex);
    /* send request to remote repository */
    if (mcc_request(rreposhandle,&hdr,comm,
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
	  pthread_mutex_unlock(&rrepos_mutex);
	  return -1;
	}
      }
      /* copy data into result buffer and adjust string pointers */
      if (comm->gc_result>0) {
	*rid = ch_alloc(ch,comm->gc_result*sizeof(MetricResourceId));
	memcpy(*rid,xbuf+sizeof(GATHERCOMM)+strlen(metricid)+1,
	       sizeof(MetricResourceId)*comm->gc_result);
	stringpool=xbuf+sizeof(GATHERCOMM)+strlen(metricid)+1+
	  comm->gc_result*sizeof(MetricResourceId);
	for (i=0;i<comm->gc_result;i++) {
	  (*rid)[i].mrid_resource = stringpool;
	  stringpool += strlen(stringpool) + 1;
	  (*rid)[i].mrid_system = stringpool;
	  stringpool += strlen(stringpool) + 1;
	}
      }
      pthread_mutex_unlock(&rrepos_mutex);
      return comm->gc_result;
    }
    pthread_mutex_lock(&rrepos_mutex);	
  }        
  return -1;
}

int rrepos_load()
{
  int pid;
  if (system("ps -C reposd")) {
    /* No reposd around */
    /*signal(SIGCHLD,SIG_IGN);*/
    pid=fork();
    switch(pid) {
    case 0:
      execlp("reposd","reposd",NULL);
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

int rrepos_unload()
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
  pthread_mutex_lock(&rrepos_mutex);
  if (mcc_request(rreposhandle,&hdr,comm,sizeof(GATHERCOMM))==0 &&
      mcc_response(&hdr,comm,&commlen)==0) {
    pthread_mutex_unlock(&rrepos_mutex);
    return comm->gc_result;
  } else {
    pthread_mutex_unlock(&rrepos_mutex);
    return -1;
  }
}

int rrepos_subscribe(SubscriptionRequest *sr,  SubscriptionCallback *scb)
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);
  off_t       offset;
  char        listener[260];

  M_TRACE(MTRACE_FLOW,MTRACE_RREPOS,
	  ("srepos_subscribe %p %p", sr, scb));
  INITCHECK();
  if (add_subscription_listener(listener,sr,scb)==-1) {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("srepos_subscribe could not set up listener"));    
    return -1;
  }
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_SUBSCRIBE;
  offset = sizeof(GATHERCOMM);
  if (marshal_string(listener,xbuf,&offset,sizeof(xbuf),1) == 0 && 
      marshal_subscriptionrequest(sr,xbuf,&offset,sizeof(xbuf)) == 0) {
    comm->gc_datalen=offset;
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rrepos_mutex);
      return comm->gc_result;
    } else {
      pthread_mutex_unlock(&rrepos_mutex);
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("srepos_subscribe remote request error"));    
      return -1;
    }
  } else {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("srepos_subscribe marshalling error, %d/%d",
	     offset,sizeof(xbuf)));    
    return -1;
  }
}

int rrepos_unsubscribe(SubscriptionRequest *sr,  SubscriptionCallback *scb)
{
  MC_REQHDR   hdr;
  char        xbuf[GATHERBUFLEN];
  GATHERCOMM *comm=(GATHERCOMM*)xbuf;
  size_t      commlen=sizeof(xbuf);
  off_t       offset;
  char        listener[260];

  M_TRACE(MTRACE_FLOW,MTRACE_RREPOS,
	  ("srepos_unsubscribe %p %p", sr, scb));
  INITCHECK();
  hdr.mc_type=GATHERMC_REQ;
  hdr.mc_handle=-1;
  comm->gc_cmd=GCMD_UNSUBSCRIBE;
  offset = sizeof(GATHERCOMM);
  current_subscription_listener(listener);
  if (marshal_string(listener,xbuf,&offset,sizeof(xbuf),1) == 0 && 
      marshal_subscriptionrequest(sr,xbuf,&offset,sizeof(xbuf)) == 0) {
    comm->gc_datalen=offset;
    pthread_mutex_lock(&rrepos_mutex);
    if (mcc_request(rreposhandle,&hdr,comm,
		    sizeof(GATHERCOMM)+comm->gc_datalen)==0 &&
	mcc_response(&hdr,comm,&commlen)==0) {
      pthread_mutex_unlock(&rrepos_mutex);
    } else {
      pthread_mutex_unlock(&rrepos_mutex);
      M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	      ("srepos_unsubscribe remote request error"));    
      return -1;
    }
  } else {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("srepos_unsubscribe marshalling error, %d/%d",
	     offset,sizeof(xbuf)));    
    return -1;
  }
  if (comm->gc_result==0 && remove_subscription_listener(listener,sr,scb)==-1) {
    M_TRACE(MTRACE_ERROR,MTRACE_RREPOS,
	    ("srepos_unsubscribe could not remove listener"));    
    return -1;
  }
  return comm->gc_result;
}
