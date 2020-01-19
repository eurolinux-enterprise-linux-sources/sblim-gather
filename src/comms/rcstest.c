/*
 * $Id: rcstest.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 *
 * Description: Server Side Communciation Test 
 *              TCP/IP Socket version
 * 
 */

#include "rcserv.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXCONN 10

static int clthdl[MAXCONN] = {0};
static pthread_t thread_id[MAXCONN];
static int connects = 0;
static pthread_mutex_t connect_mutex = PTHREAD_MUTEX_INITIALIZER;


/* ---------------------------------------------------------------------------*/

static void * _get_request(void *hdl)
{
  long   rhdl;
  int    i = 0;
  char   buf[500];
  size_t buflen;

  rhdl = (long)hdl;
  fprintf(stderr,"--- start thread on socket %i\n",(int)rhdl);
  
  while (1) {
    buflen = sizeof(buf);
    if (rcs_getrequest((int)rhdl,buf,&buflen) == -1) {
      fprintf(stderr,"--- time out on socket %i\n",(int)rhdl);
      break;
    }
    fprintf(stderr,"---- received on socket %i: %s\n",(int)rhdl,buf);
  }

  pthread_mutex_lock(&connect_mutex);
  for(i=0;i<MAXCONN;i++) {
    if (clthdl[i] == (int)rhdl) {
      clthdl[i] = -1;
      connects--;
      break;
    }
  }
  pthread_mutex_unlock(&connect_mutex);
  fprintf(stderr,"--- exit thread on socket %i\n",(int)rhdl);
  return NULL;
}


/* ---------------------------------------------------------------------------*/

int main()
{
  int hdl             = -1;
  long thdl           = -1;
  int port            = 6363;
  int i               = 0;
  struct timespec req = {0,0};
  struct timespec rem = {0,0};

  memset(thread_id,0,sizeof(thread_id));

  if (rcs_init(&port)) {
    return -1;;
  }

  while (1) {
    pthread_mutex_lock(&connect_mutex);
    if (hdl == -1) {
      if (rcs_accept(&hdl) == -1) { return -1;}
    }
    for(i=0;i<MAXCONN;i++) { 
      if (clthdl[i] <= 0) {
	clthdl[i] = hdl;
	break;
      } 
    }
    thdl = hdl;
    if (pthread_create(&thread_id[i],NULL,_get_request,(void *)thdl) != 0) {
      perror("create thread");
      return -1;
    }
    hdl = -1;
    fprintf(stderr,"thread_id [%i] : %ld\n",i,thread_id[i]);
    if (connects<(MAXCONN-1)) { connects++; }
    else {
      pthread_mutex_unlock(&connect_mutex);
      /* wait until at least one thread finished */
      while (1) {
	req.tv_sec = 0;
	req.tv_nsec = 100;
	nanosleep(&req,&rem);
	pthread_mutex_lock(&connect_mutex);
	if(connects<(MAXCONN-1)) { break; }
	pthread_mutex_unlock(&connect_mutex);
      }
    }
    pthread_mutex_unlock(&connect_mutex);
  }
  rcs_term();
  return 0;
}


/* ---------------------------------------------------------------------------*/
/*                              end of rcstest.c                              */
/* ---------------------------------------------------------------------------*/
