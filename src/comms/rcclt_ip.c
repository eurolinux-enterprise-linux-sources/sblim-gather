/*
 * $Id: rcclt_ip.c,v 1.9 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: Heidi Neumann <heidineu@de.ibm.com>
 *
 * Description:  Gatherer Client-Side Communication APIs
 *               TCP/IP Socket version
 *
 */

#include "rcclt.h"

#include <mtrace.h>
#include <merrno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ---------------------------------------------------------------------------*/

static struct sockaddr_in srvaddr;
static int srvhdl;
static int connects;
static int requests;

static int _sigpipe_h_installed = 0;
static int _sigpipe_h_received = 0;

static void _sigpipe_h(int __attribute__ ((unused)) signal)
{
  _sigpipe_h_received++;
}

/* ---------------------------------------------------------------------------*/

int rcc_init(const char *srvid, const int *portid)
{
  struct hostent *srv = NULL;
  char   ip[16];
  struct sigaction sigact;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcc_init(%s,%d) called",srvid,*portid));
  if (srvid && portid && srvaddr.sin_port==0) {

    /* retrieve and set server information */
    if ((srv = gethostbyname(srvid)) == NULL) {
      m_seterrno(MC_ERR_BADINIT);
      m_setstrerror("rcc_init %s: %s",hstrerror(h_errno),srvid);
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_init %s: %s",hstrerror(h_errno),srvid));
      return -1; 
    }
    sprintf(ip, "%u.%u.%u.%u",
	    ((unsigned char *)srv->h_addr)[0],
	    ((unsigned char *)srv->h_addr)[1],
	    ((unsigned char *)srv->h_addr)[2],
	    ((unsigned char *)srv->h_addr)[3]);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port   = htons(*portid);
    if (inet_aton(ip,&srvaddr.sin_addr) == 0) {
      m_seterrno(MC_ERR_BADINIT);
      m_setstrerror("rcc_init server address not valid: %s",inet_ntoa(srvaddr.sin_addr));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_init server address not valid: %s",inet_ntoa(srvaddr.sin_addr)));
      return -1;
    }

    /* install signal handler */
    if (!_sigpipe_h_installed) {
      memset(&sigact,0,sizeof(sigact));
      sigact.sa_handler = _sigpipe_h;            
      sigact.sa_flags = 0;
      sigaction(SIGPIPE,&sigact,NULL);
    }

    /* init global variables */
    srvhdl=-1;
    connects=0;
    requests=0;

    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("rcc_init initialized for %s:%d",inet_ntoa(srvaddr.sin_addr),*portid));
    return 0;
  }

  m_seterrno(MC_ERR_BADINIT);
  if (srvaddr.sin_port!=0) {
    m_setstrerror("rcc_init client already initialized for %s",inet_ntoa(srvaddr.sin_addr));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("rcc_init client already initialized for %s",inet_ntoa(srvaddr.sin_addr)));
  }
  else {
    if (!srvid && !portid) {
      m_setstrerror("rcc_init server and port not defined");
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_init server and port not defined"));
    }
    else if (!srvid) {
      m_setstrerror("rcc_init server not defined");
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_init server not defined"));
    }
    else {
      m_setstrerror("rcc_init port not defined");
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_init port not defined"));
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/

int rcc_term()
{
  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcc_term() called"));
  if (srvhdl > 0) {
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("rcc_term closing socket %d for %s",
	     srvhdl,inet_ntoa(srvaddr.sin_addr)));
    close(srvhdl);
    srvhdl = -1;
    memset(&srvaddr,0,sizeof(struct sockaddr_in));
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,("rcc_term() succeeded"));
    return 0;
  }
  m_seterrno(MC_ERR_INVHANDLE);
  m_setstrerror("rcc_term client already down");
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,("rcc_term client already down"));
  return -1;
}


/* ---------------------------------------------------------------------------*/

int _rcc_connect() 
{
  int connhdl;

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("_rcc_connect() called"));
  if (srvhdl > 0) {
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,("_rcc_connect closing socket %d",srvhdl));
    close(srvhdl);
  }

  srvhdl=socket(PF_INET, SOCK_STREAM, 0);
  if (srvhdl==-1) {
    m_seterrno(MC_ERR_NOCONNECT);
    m_setstrerror("_rcc_connect could not create socket for %s,"
		  " system error string: %s",
		  inet_ntoa(srvaddr.sin_addr),strerror(errno));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("_rcc_connect could not create socket for %s,"
	     " system error string: %s",
	     inet_ntoa(srvaddr.sin_addr),strerror(errno)));
    return -1;
  }

  connhdl = connect(srvhdl,(struct sockaddr*)&srvaddr,
		    sizeof(struct sockaddr_in));
 
  if (connhdl < 0) {
    m_seterrno(MC_ERR_NOCONNECT);
    m_setstrerror("_rcc_connect could not connect socket for %s,"
		  " system error string: %s",
		  inet_ntoa(srvaddr.sin_addr),strerror(errno));
    M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	    ("_rcc_connect could not connect socket for %s,"
	     " system error string: %s",
	     inet_ntoa(srvaddr.sin_addr),strerror(errno)));
    close(srvhdl);
    srvhdl=-1;
  }
  M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("_rcc_connect opened socket %d",srvhdl));
  connects++;
  return connhdl;
}

/* ---------------------------------------------------------------------------*/

int rcc_request(void *reqdata, size_t reqdatalen)
{
  ssize_t sentlen = 0;
  size_t  reqlen  = 0;
  RC_SIZE_T wirelen = htonl(reqdatalen);

  M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	  ("rcc_request(%p,%u) called",reqdata,reqdatalen));
  if (reqdata == NULL) {
    m_seterrno(MC_ERR_INVPARAM);
    m_setstrerror("rcc_request received invalid parameter (%p)",reqdata);
    M_TRACE(MTRACE_FLOW,MTRACE_COMM,
	    ("rcc_request received invalid parameter (%p)",reqdata));
    return -1;
  }
  
  if (srvhdl <= 0) {
    if (_rcc_connect() < 0) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("rcc_request could not connect socket for %s,"
		    " system error string: %s",
		    inet_ntoa(srvaddr.sin_addr),strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_request could not connect socket for %s,"
	       " system error string: %s",
	       inet_ntoa(srvaddr.sin_addr),strerror(errno)));
      return -1;
    }
  }

  reqlen = reqdatalen+sizeof(RC_SIZE_T);
  M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("rcc_request socket %d write",srvhdl));
  sentlen = write(srvhdl,&wirelen,sizeof(RC_SIZE_T)) +
    write(srvhdl,reqdata,reqdatalen);

  if (sentlen <= 0) {
    if (_rcc_connect() < 0) {
      m_seterrno(MC_ERR_NOCONNECT);
      m_setstrerror("rcc_request could not reconnect socket for %s,"
		    " system error string: %s",
		    inet_ntoa(srvaddr.sin_addr),strerror(errno));
      M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	      ("rcc_request could not reconnect socket for %s,"
	       " system error string: %s",
	       inet_ntoa(srvaddr.sin_addr),strerror(errno)));
      return -1;
    }
    else {
      M_TRACE(MTRACE_DETAILED,MTRACE_COMM,("rcc_request socket %d retry write",srvhdl));
      sentlen = write(srvhdl,&wirelen,sizeof(RC_SIZE_T)) +
	write(srvhdl,reqdata,reqdatalen);
    }
  }

  if ((size_t) sentlen == reqlen) { 
    M_TRACE(MTRACE_DETAILED,MTRACE_COMM,
	    ("rcc_request on socket %d for %s succeeded, count=%d",
	     srvhdl,inet_ntoa(srvaddr.sin_addr),requests++));
    requests++;
    return 0; 
  }

  m_seterrno(MC_ERR_IOFAIL);
  m_setstrerror("rcc_request send error, wanted %d got %d,"
		" system error string: %s",
		reqdatalen+sizeof(RC_SIZE_T),sentlen,strerror(errno));
  M_TRACE(MTRACE_ERROR,MTRACE_COMM,
	  ("rcc_request send error, wanted %d got %d,"
	   " system error string: %s",
	   reqdatalen+sizeof(RC_SIZE_T),sentlen,strerror(errno)));
  return -1;
}


/* ---------------------------------------------------------------------------*/
/*                             end of rcclt_ip.c                              */
/* ---------------------------------------------------------------------------*/
