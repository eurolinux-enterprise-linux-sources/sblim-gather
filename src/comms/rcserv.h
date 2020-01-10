/*
 * $Id: rcserv.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: Heidi Neumann <heidineu@de.ibm.com>
 *
 * Description: Communication Server APIs
 *              TCP/IP Socket version
 *
 */

#ifndef RCSERV_H
#define RCSERV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "mcdefs.h"

int rcs_init(const int *portid);
int rcs_term();
int rcs_accept(int *clthdl);
int rcs_getrequest(int clthdl, void *reqdata, size_t *reqdatalen);

#ifdef __cplusplus
}
#endif

#endif
