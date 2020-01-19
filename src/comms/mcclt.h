/*
 * $Id: mcclt.h,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Communication Client APIs
 */

#ifndef MCCLT_H
#define MCCLT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>  
#include "mcdefs.h"

int mcc_init(const char *commid);
int mcc_term(int commhandle);
int mcc_request(int commhandle, MC_REQHDR *hdr, void *reqdata, 
		size_t reqdatalen);
int mcc_response(MC_REQHDR *hdr, void *respdata, size_t *respdatalen);

#ifdef __cplusplus
}
#endif

#endif
