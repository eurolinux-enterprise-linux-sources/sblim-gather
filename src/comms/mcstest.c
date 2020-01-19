/*
 * $Id: mcstest.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2009
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
 * Description: Server Side Communciation Test 
 * 
 */

#include "mcserv.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
  MC_REQHDR  req;
  char       buf[500];
  size_t     buflen;
  
  if (mcs_init("mcstest") == 0) {
    while (1) {
      /* keep on living */
      if (mcs_accept(&req) == -1) {
	return -1;
      }
      do {
	buflen=sizeof(buf);
	if (mcs_getrequest(&req,buf,&buflen) == -1) {
	  break;
	}
	/*      if (req.mc_len>0)
		puts(req.mc_data);*/
	if (req.mc_type!=0) {
	  buflen = 5;
	  strcpy(buf,"JUHU");
	  if (mcs_sendresponse(&req,buf,buflen) == -1) {
	  break;
	  }
	} else {
	  break;
	}
      } while (1);
    }
    mcs_term();
  }  
  return 0;
}


