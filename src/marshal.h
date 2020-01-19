/*
 * $Id: marshal.h,v 1.4 2009/05/20 19:39:55 tyreld Exp $
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
 * Description: Marshalling
 */

#ifndef MARSHAL_H
#define MARSHAL_H

#include "repos.h"

#ifdef __cplusplus
extern "C" {
#endif

int marshal_string(const char *str, char *mbuf, off_t *offset, size_t mbuflen, 
		   int required);
int unmarshal_string(char **str, const char *mbuf, off_t *offset, 
		     size_t mbuflen, int required);

int marshal_data(const void *data, size_t datalen, char *mbuf, 
		 off_t *offset, size_t mbuflen);
int unmarshal_data(void **data, size_t datalen, const char *mbuf, 
		   off_t *offset, size_t mbuflen);

int unmarshal_fixed(void *data, size_t datalen, const char *mbuf, 
		    off_t *offset, size_t mbuflen);

int marshal_valuerequest(const ValueRequest *vr, char *mbuf, 
			 off_t * offset, size_t mbuflen);
int unmarshal_valuerequest(ValueRequest **vr, const char *mbuf, 
			   off_t * offset, size_t mbuflen);

int marshal_subscriptionrequest(const SubscriptionRequest *vr, char *mbuf,
				off_t * offset, size_t mbuflen);
int unmarshal_subscriptionrequest(SubscriptionRequest **vr, const char *mbuf, 
				  off_t * offset, size_t mbuflen);

int marshal_reposplugindefinition(const RepositoryPluginDefinition *rdef, 
				  size_t num, char *mbuf,
				  off_t * offset, size_t mbuflen);
int unmarshal_reposplugindefinition(RepositoryPluginDefinition **rdef, 
				    size_t num, char *mbuf,
				    off_t * offset, size_t mbuflen);

#ifdef __cplusplus
}
#endif

#endif
