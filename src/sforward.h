/*
 * $Id: sforward.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: 
 *
 * Description: Subscription Forwarding
 */

#ifndef SFORWARD_H
#define SFORWARD_H

#include "repos.h"

#ifdef __cplusplus
extern "C" {
#endif

int subs_enable_forwarding(SubscriptionRequest *sr, const char *listenerid);
int subs_disable_forwarding(SubscriptionRequest *sr, const char *listenerid);
void subs_forward(int corrid, ValueRequest *vr);

#ifdef __cplusplus
}
#endif

#endif
