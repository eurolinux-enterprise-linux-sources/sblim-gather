/*
 * $Id: mcserv.h,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Communication Server APIs
 */

#ifndef MCSERV_H
#define MCSERV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "mcdefs.h"

int mcs_init(const char *commid);
int mcs_term();
int mcs_accept(MC_REQHDR *hdr);
int mcs_getrequest(MC_REQHDR *hdr, void *reqdata, size_t *reqdatalen);
int mcs_sendresponse(MC_REQHDR *hdr, void *respdata, size_t respdatalen);

#ifdef __cplusplus
}
#endif

#endif
