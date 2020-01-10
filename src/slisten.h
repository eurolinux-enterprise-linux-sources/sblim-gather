/*
 * $Id: slisten.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Subscription Listening
 */

#ifndef SLISTEN_H
#define SLISTEN_H

#include "repos.h"

#ifdef __cplusplus
extern "C" {
#endif

int add_subscription_listener(char *listenerid, SubscriptionRequest *sr,
			      SubscriptionCallback * scb);

int remove_subscription_listener(const char *listenerid, 
				 SubscriptionRequest *sr,
				 SubscriptionCallback * scb);

int current_subscription_listener(char *listenerid);

#ifdef __cplusplus
}
#endif

#endif
