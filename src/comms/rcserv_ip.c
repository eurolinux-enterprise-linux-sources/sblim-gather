/*
 * $Id: rcserv_ip.c,v 1.11 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors: Heidi Neumann <heidineu@de.ibm.com>
 *               Michael Schuele <schuelem@de.ibm.com>
 *
 * Description: Gatherer Server-Side Communication APIs
 *              TCP/IP Socket version
 *
 */

#include "rcserv.h"

#include <mtrace.h>
#include <merrno.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>


/* ---------------------------------------------------------------------------*/
#ifndef HOST_NAME_MAY
#define HOST_NAME_MAX  64
#endif
#define PORT 6363

static int srvhdl=-1;
static int connects;

static pthread_mutex_t connect_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _sigpipe_h_installed = 0;
static int _sigpipe_h_received = 0;

static void _sigpipe_h(int signal)
{
  _sigpipe_h_received++; 
}


/* ---------------------------------------------------------------------------*/

int rcs_init(const int *portid)
{
  struct sockaddr_in srvaddr;
  struct sigaction sigact;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcs_init(%d) called",*portid));
  if (srvhdl != -1) {
    m_seterrno(MC_ERR_BADINIT);
    m_setstrerror("rcs_init server already initialized");
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,("rcs_init server already initialized"));
    return -1;
  }

  /* setup address info */
  srvaddr.sin_family = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (!portid) { srvaddr.sin_port = htons(PORT); }
  else { srvaddr.sin_port = htons(*portid); }

  /* create socket */
  srvhdl=socket(PF_INET, SOCK_STREAM, 0);
  if (srvhdl==-1) {
    m_seterrno(MC_ERR_NOCONNECT);
    m_setstrerror("rcs_init could not create socket for %s,"
		  " system error string: %s",
		  inet_ntoa(srvaddr.sin_addr),strerror(errno));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcs_init could not create socket for %s,"
	     " system error string: %s",
	     inet_ntoa(srvaddr.sin_addr),strerror(errno)));
    return -1;
  }

  /* bind socket */
  if (bind(srvhdl,(struct sockaddr*)&srvaddr,sizeof(srvaddr))) {
    m_seterrno(MC_ERR_NOCONNECT);
    m_setstrerror("rcs_init could not bind socket %d for %s,"
		  " system error string: %s",
		  srvhdl,inet_ntoa(srvaddr.sin_addr),strerror(errno));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcs_init could not create socket %d for %s,"
	     " system error string: %s",
	     srvhdl,inet_ntoa(srvaddr.sin_addr),strerror(errno)));
    close(srvhdl);
    return -1;
  }

  /* install signal handler */
  if (!_sigpipe_h_installed) {
    memset(&sigact,0,sizeof(sigact));    
    sigact.sa_handler = _sigpipe_h;            
    sigact.sa_flags = 0;
    sigaction(SIGPIPE,&sigact,NULL);
  }

  /* listen on socket */
  if (listen(srvhdl,0) < 0) {
    m_seterrno(MC_ERR_NOCONNECT);
    m_setstrerror("rcs_init could not listen on socket %d for %s,"
		  " system error string: %s",
		  srvhdl,inet_ntoa(srvaddr.sin_addr),strerror(errno));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcs_init could not listen on socket %d for %s,"
	     " system error string: %s",
	     srvhdl,inet_ntoa(srvaddr.sin_addr),strerror(errno)));
    close(srvhdl);
    return -1;
  }

  connects=0;  
  M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	  ("rcs_init initialized for %s:%d",inet_ntoa(srvaddr.sin_addr),*portid));
  return 0;
}


/* ---------------------------------------------------------------------------*/

int rcs_term()
{
  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcs_term() called"));
  if (srvhdl != -1) {
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("rcs_term closing socket %s",srvhdl));
    close(srvhdl);
    srvhdl = -1;
    _sigpipe_h_installed=0;
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcs_term succeeded"));
    return 0;
  }
  m_seterrno(MC_ERR_INVHANDLE);
  m_setstrerror("rcs_term server already down");
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,("rcs_term server already down"));
  return -1;
}


/* ---------------------------------------------------------------------------*/

int rcs_accept(int *clthdl)
{
  struct linger optval = {1};
  socklen_t optlen = sizeof(optval);

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcs_accept() called"));
  pthread_mutex_lock(&connect_mutex);
  if (srvhdl == -1) {
    m_seterrno(MC_ERR_BADINIT);
    m_setstrerror("rcs_accept server not initialized");
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcs_accept server not initialized"));
  }
  if (clthdl) {
    *clthdl=accept(srvhdl,NULL,0);
    if (*clthdl == -1) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("rcs_accept could not accept on socket %d,"
		    " system error string: %s",
		    srvhdl,strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcs_accept could not accept on socket %d,"
	       " system error string: %s",
	       srvhdl,strerror(errno)));
      pthread_mutex_unlock(&connect_mutex);
      return -1;
    }

    if (setsockopt(*clthdl,SOL_SOCKET,SO_LINGER,(void*)&optval,optlen) != 0) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("rcs_accept could not set LINGER on socket %d,"
		    " system error string: %s",
		    srvhdl,strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcs_accept could not set LINGER on socket %d,"
	       " system error string: %s",
	       srvhdl,strerror(errno)));
    }
    
    connects++;
    pthread_mutex_unlock(&connect_mutex);
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcs_accept opened socket %d to client",*clthdl));
    return 0;
  }
  pthread_mutex_unlock(&connect_mutex);
  m_seterrno(MC_ERR_INVPARAM);
  m_setstrerror("rcs_accept received invalid parameter (%p)",clthdl);
  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	  ("rcs_accept received invalid parameter (%p)",clthdl));
  return -1;
}


/* ---------------------------------------------------------------------------*/

int rcs_getrequest(int clthdl, void *reqdata, size_t *reqdatalen)
{
  struct pollfd pf;
  ssize_t   readlen = 0;
  size_t    recvlen = 0;
  size_t    maxlen  = 0;
  RC_SIZE_T wirelen;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	  ("rcs_getrequest(%d,%p,%u) called",clthdl,reqdata,reqdatalen));

  if (srvhdl == -1) {
    m_seterrno(MC_ERR_BADINIT);
    m_setstrerror("rcs_getrequest server not initialized");
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcs_getrequest server not initialized"));
    return -1;
  }
  if (reqdata == NULL) {
    m_seterrno(MC_ERR_INVPARAM);
    m_setstrerror("rcs_getrequest received invalid parameter (%p)",reqdata);
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	    ("rcs_getrequest received invalid parameter (%p)",reqdata));
    return -1;
  }

  if ((clthdl > 0) && reqdatalen) {
    pf.fd = clthdl;
    pf.events=POLLIN;
    if (poll(&pf,1,1000) == 1 && 
	!(pf.revents & (POLLERR|POLLNVAL|POLLHUP))) {
      if (*reqdatalen > 0) { maxlen=*reqdatalen; }
      do {
	/* get length */
	readlen=read(clthdl,(char*)&wirelen+recvlen,sizeof(RC_SIZE_T)-recvlen);

	if (readlen <= 0) { break; }
	recvlen += readlen;
      } while (recvlen != sizeof(RC_SIZE_T));
      *reqdatalen = ntohl(wirelen);

      if (maxlen > 0 && *reqdatalen > maxlen) {
	m_seterrno(MC_ERR_IOFAIL);
	m_setstrerror("rcs_getrequest socket %d read error, wanted %d got %d,"
		      " system error string: %s",
		      clthdl,*reqdatalen,readlen,strerror(errno));
	M_TRACE(MTRACE_ERROR,MTRACE_COMM,
		("rcs_getrequest socket %d read error, wanted %d got %d,"
	       " system error string: %s",
		 clthdl,*reqdatalen,readlen,strerror(errno)));
      } else if (readlen > 0) {
	readlen=0;
	recvlen=0;
	do {
	  /* get data */
	  readlen=read(clthdl,(char*)reqdata+recvlen,*reqdatalen-recvlen);
	  if (readlen <= 0) { break; }
	  recvlen += readlen;
	} while (recvlen != *reqdatalen);
	
	if (readlen > 0) {
	  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
		  ("rcs_getrequest socket %d retrieved data",clthdl));
	  return 0; 
	}
      }
    }

    if (readlen < 0) {
      m_seterrno(MC_ERR_IOFAIL);
      m_setstrerror("rcs_getrequest socket %d read error %d,"
		    " system error string: %s",
		    clthdl,readlen,strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcs_getrequest socket %d read error %d,"
	       " system error string: %s",
	       clthdl,readlen,strerror(errno)));
    }
  }

  if (clthdl > 0) { 
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("rcs_getrequest closing socket %d",clthdl));
    close(clthdl); 
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/*                              end of rcserv_ip.c                            */
/* ---------------------------------------------------------------------------*/
