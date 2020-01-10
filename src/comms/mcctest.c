/*
 * $Id: mcctest.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Client Side Test Program
 */

#include "mcclt.h"
#include <mtrace.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

static int transactions = 0;

#define FACTOR 10
#define PAUSE  3

int main()
{
  MC_REQHDR  req = {0,-1};
  char   buf[500];
  size_t buflen;
  time_t start, end;
  int    comhdl;

#ifndef NOTRACE
  m_trace_setlevel(MTRACE_ALL);
  m_trace_setfile("mcctest.trc");
  m_trace_enable(MTRACE_MASKALL);
#endif
  start=time(NULL);
  comhdl = mcc_init("mcstest");
  memset(buf,0,sizeof(buf));

  strcpy(buf,"x");
  do {
    /* fgets(buf,sizeof(buf),stdin); */
    if (*buf=='q') {
      req.mc_type=0;
      mcc_request(comhdl,&req,"",0);
      break;
    } else {
      req.mc_type=1;
      buflen=sizeof(buf);
      if (mcc_request(comhdl,&req,buf,strlen(buf))==0) {
	mcc_response(&req,buf,&buflen);	
	if (transactions == 100 * FACTOR) {
	  sleep(PAUSE); /* stall processing */
	}
	if (transactions++ == 500 * FACTOR) {
	  end = time(NULL);
	  fprintf(stderr,"Transactions %d in seconds %ld, rate = %ld\n",
		  transactions,end-start-PAUSE,transactions/(end-start-PAUSE));
	  break;
	}
	/*puts(buf);*/
      } else {
	break;
      }
    }
  } while(1);

  mcc_term(comhdl);
  return 0;
}
