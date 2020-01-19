/*
 * $Id: mcclt_unix.c,v 1.16 2012/10/27 00:52:12 tyreld Exp $
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
 * Contributors: Michael Schuele <schuelem@de.ibm.com> 
 *
 * Description:  Gatherer Client-Side Communication APIs
 * AF_UNIX version.
 *
 */

#include "mcclt.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <mtrace.h>
#include <merrno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

#define MAXCONN 10

static int _sigpipe_h_installed = 0;
static int _sigpipe_h_received = 0;

static void _sigpipe_h(int __attribute__ ((unused)) signal)
{
  _sigpipe_h_received++; 
}

/* the mutexes only protect creation/deletion - not the I/O-s */
/* this must be done in the app layer */
static pthread_mutex_t sockname_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct {
  char sn_name[PATH_MAX+2];
  int  sn_handle;
  /* runtime statistics */
  int  sn_connects;
  int  sn_requests;
} sockname[MAXCONN] = {{{0},0,0,0}};

/* writev helper for partial writes */
static int mc_writev(int handle, struct iovec *iov, ssize_t numblock);

int mcc_init(const char *commid)
{
  int i;
  struct sigaction sigact;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("mcc_init(%s) called",commid));
  pthread_mutex_lock(&sockname_mutex);
  for (i=0; i < MAXCONN;i++) {
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_init search free handle"));
    if (sockname[i].sn_name[0]==0) {
      M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_init found free handle %d",i));
      if (snprintf(sockname[i].sn_name,PATH_MAX+2,MC_SOCKET,commid) > 
	  PATH_MAX) {
	m_seterrno(MC_ERR_BADINIT);
	m_setstrerror("mcc_init could not complete socket name %s",commid);
	M_TRACE(MTRACE_ERROR,MTRACE_COMM,
		("mcc_init could not complete socket name %s"));
        /* TODO do we need a mutex unlock here? */
	return -1;
      }
      if (!_sigpipe_h_installed) {
	memset(&sigact,0,sizeof(sigact));
	sigact.sa_handler = _sigpipe_h;
	sigact.sa_flags = 0;
	sigaction(SIGPIPE,&sigact,NULL);
        /* TODO does _sigpipe_h_installed need to be set to true? */
      }
      pthread_mutex_unlock(&sockname_mutex);
      M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_init return handle %d",i));
      return i;
    }
  }
  pthread_mutex_unlock(&sockname_mutex);
  m_seterrno(MC_ERR_BADINIT);
  m_setstrerror("mcc_init out of socket slots for %s",commid);
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("mcc_init out of socket slots for %s",commid));
  return -1;
}

int mcc_term(int commhandle)
{
  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("mcc_term(%d) called",commhandle));
  pthread_mutex_lock(&sockname_mutex);
  if (commhandle >= 0 && commhandle < MAXCONN && 
      sockname[commhandle].sn_name[0]) {
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("mcc_term closing socket %s",sockname[commhandle].sn_name));
    sockname[commhandle].sn_name[0]=0;
    if (sockname[commhandle].sn_handle > 0) {
      close(sockname[commhandle].sn_handle);
      sockname[commhandle].sn_handle=-1;
    }
    pthread_mutex_unlock(&sockname_mutex);
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_term freed handle %d",commhandle));
    return 0;
  }
  pthread_mutex_unlock(&sockname_mutex);
  m_seterrno(MC_ERR_INVHANDLE);
  m_setstrerror("mcc_term received invalid handle %d",
		commhandle);
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("mcc_term received invalid handle %d",commhandle));
  return -1;
}

static int _mcc_connect(int commhandle)
{    
  struct sockaddr_un sa;
  int connhandle;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("_mcc_connect(%d) called",commhandle));
  if (commhandle < MAXCONN && sockname[commhandle].sn_name[0]) {
    if (sockname[commhandle].sn_handle>0) {
      M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	      ("_mcc_connect closing %s",sockname[commhandle].sn_name));
      close(sockname[commhandle].sn_handle);
    }
    sockname[commhandle].sn_handle=socket(PF_UNIX,SOCK_STREAM,0);
    if (sockname[commhandle].sn_handle==-1) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("_mcc_connect could not create socket for %s,"
		    " system error string: %s",
		    sockname[commhandle].sn_name,
		    strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("_mcc_connect could not create socket for %s,"
	       " system error string: %s",
	       sockname[commhandle].sn_name,
	       strerror(errno)));
      return -1;
    }
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path,sockname[commhandle].sn_name);
    sockname[commhandle].sn_connects ++;
    connhandle=connect(sockname[commhandle].sn_handle,
		       (struct sockaddr*)&sa,
		       sizeof(struct sockaddr_un));
    if (connhandle < 0) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("_mcc_connect could not connect socket for %s,"
		    " system error string: %s",
		    sockname[commhandle].sn_name,
		    strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("_mcc_connect could not connect socket for %s,"
	       " system error string: %s",
	       sockname[commhandle].sn_name,
	       strerror(errno)));
    }
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("_mcc_connect returning %d",connhandle));
    return connhandle;
  }
  /* invalid commhandle */
  m_seterrno(MC_ERR_INVHANDLE);
  m_setstrerror("_mcc_connect received invalid handle %d",
		commhandle);
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("_mcc_connect received invalid handle %d",commhandle));
  return -1;
}

int mcc_request(int commhandle, MC_REQHDR *hdr, 
		void *reqdata, size_t reqdatalen)
{
  int        sentlen;
  struct iovec iov [3] = { 
    {hdr, sizeof(MC_REQHDR)}, 
    {&reqdatalen, sizeof(size_t)}, 
    {reqdata, reqdatalen}, 
  };

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	  ("mcc_request(%d,%p,%p,%u) called",
	   commhandle,hdr,reqdata,reqdatalen));
  if (commhandle < 0 || commhandle >= MAXCONN || hdr == NULL || 
      reqdata == NULL) {
    m_seterrno(MC_ERR_INVPARAM);
    m_setstrerror("mcc_request received invalid parameters (%d,%p,%p)",
		  commhandle,hdr,reqdata);
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	    ("mcc_request received invalid parameters (%d,%p,%p)",
	     commhandle,hdr,reqdata));
    return -1;
  }
  if (sockname[commhandle].sn_handle<=0) {
    if (_mcc_connect(commhandle)<0 ) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("mcc_request could not connect socket for %s,"
		    " system error string: %s\n",
		    sockname[commhandle].sn_name,
		    strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("mcc_request could not connect socket for %s,"
	       " system error string: %s\n",
	       sockname[commhandle].sn_name,
	       strerror(errno)));
      return -1;
    }
  }
  M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_request write"));
  sentlen = mc_writev(sockname[commhandle].sn_handle,iov,3);
  if (sentlen <= 0) {
    if (_mcc_connect(commhandle)<0 ) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("mcc_request could not reconnect socket for %s,"
		    " system error string: %s\n",
		    sockname[commhandle].sn_name,
		    strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("mcc_request could not reconnect socket for %s,"
		    " system error string: %s\n",
		    sockname[commhandle].sn_name,
		    strerror(errno)));
      return -1;
    } else {
      M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_request retry write"));
      sentlen = mc_writev(sockname[commhandle].sn_handle,iov,3);
    }
  }
  if ((size_t) sentlen == (reqdatalen+sizeof(size_t)+sizeof(MC_REQHDR))) {
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("mcc_request for %s succeeded, count=%d",
	     sockname[commhandle].sn_name,
	     sockname[commhandle].sn_requests++));
    hdr->mc_handle=sockname[commhandle].sn_handle;
    sockname[commhandle].sn_requests++;
    return 0;
  }
  m_seterrno(MC_ERR_IOFAIL);
  m_setstrerror("mcc_request send error, wanted %d got %d,"
		" system error string: %s\n",
		reqdatalen+sizeof(size_t)+sizeof(MC_REQHDR),
		sentlen, strerror(errno));
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("mcc_request send error, wanted %d got %d,"
	   " system error string: %s\n",
	   reqdatalen+sizeof(size_t)+sizeof(MC_REQHDR),
	   sentlen, strerror(errno)));
  return -1;
}

int mcc_response(MC_REQHDR *hdr, void *respdata, size_t *respdatalen)
{
  struct iovec iov[2] = {
    {hdr,sizeof(MC_REQHDR)},
    {respdatalen,sizeof(size_t)}
  };
  int    cltsock;
  int    readlen=0;
  size_t  recvlen=0;
  size_t  maxlen=0;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	  ("mcc_response(%p,%p,%u) called",hdr,respdata,respdatalen));
  if (hdr==NULL || hdr->mc_handle ==-1 || respdata == NULL || 
      respdatalen == NULL) {
    m_seterrno(MC_ERR_INVPARAM);
    m_setstrerror("_mcc_response received invalid parameters (%p,%d,%p)",
		  hdr,hdr->mc_handle,respdata);
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("_mcc_response received invalid parameters (%p,%d,%p)",
	     hdr,hdr->mc_handle,respdata));
    return -1;
  }
  if (*respdatalen>0) maxlen=*respdatalen;
  cltsock = hdr->mc_handle;
  do {
    /* get header & size */
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_response reading header chunk"));
    readlen=readv(cltsock,iov,2);
    if (readlen <= 0) {
      m_seterrno(MC_ERR_IOFAIL);
      m_setstrerror("mcc_request read error, wanted %d got %d,"
		    " system error string: %s\n",
		    sizeof(size_t)+sizeof(MC_REQHDR),
		    readlen, strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("mcc_request read error, wanted %d got %d,"
	       " system error string: %s\n",
	       sizeof(size_t)+sizeof(MC_REQHDR),
	       readlen, strerror(errno)));
      return -1;
    }
    recvlen += readlen;
    if (iov[0].iov_len < (size_t) readlen) {
      readlen -= iov[0].iov_len;
      iov[0].iov_len=0; 
      iov[1].iov_len-=readlen;
    } else {
      iov[0].iov_len-=readlen; 
    }
  } while (recvlen != (sizeof(MC_REQHDR)+sizeof(size_t)));
  /* restore mc_handle to avoid accidental use of server's handle */
  hdr->mc_handle = cltsock;
  M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	  ("mcc_response done reading header, data length=%d",*respdatalen));
  if (maxlen > 0 && *respdatalen > maxlen) {
    m_seterrno(MC_ERR_OVERFLOW);
    m_setstrerror("mcc_response buffer to small, needed %d available %d",
		  *respdatalen,
		  maxlen);
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("mcc_response buffer to small, needed %d available %d",
	     *respdatalen,
	     maxlen)); 
    /* we close the socket to discard superfluous data */
    close(cltsock);
    return -1;
  }  
  readlen=0;
  recvlen=0;
  do {
    /* get data */
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_response reading data chunk"));
    readlen=read(cltsock,(char*)respdata+recvlen,*respdatalen-recvlen);
    if (readlen <= 0) break;
    recvlen+=readlen;
  } while (recvlen != *respdatalen);
  if (readlen > 0) {      
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("mcc_response done reading data"));
    return 0;
  }
  m_seterrno(MC_ERR_IOFAIL);
  m_setstrerror("mcc_request unexpected read error, "
		" system error string: %s",
		strerror(errno));
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("mcc_request unexpected read error, "
	   " system error string: %s",
	   strerror(errno)));
  return -1;
}

static int mc_writev(int handle, struct iovec *iov, ssize_t numblock)
{
  int startblock = 0;
  int writelen = 0;
  int sentlen = 0;
  do {
    while (startblock < numblock) {
      /* advance start block and offset */
      if ((ssize_t)iov[startblock].iov_len - writelen < 0) {
	writelen -= iov[startblock].iov_len;
	startblock +=1;
      } else {
	iov[startblock].iov_base += writelen;
	iov[startblock].iov_len -= writelen;
	break;
      }
    }
    writelen = writev(handle,iov+startblock,numblock-startblock);
    sentlen += writelen;
  } while (writelen > 0 && startblock < numblock );
  return sentlen;
}
