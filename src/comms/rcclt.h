/*
 * $Id: rcclt.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Communication Client APIs
 *              TCP/IP Socket version
 *
 */

#ifndef RCCLT_H
#define RCCLT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "mcdefs.h"

int rcc_init(const char *srvid, const int *portid);
int rcc_term();
int rcc_request(void *reqdata, size_t reqdatalen);

#ifdef __cplusplus
}
#endif

#endif
