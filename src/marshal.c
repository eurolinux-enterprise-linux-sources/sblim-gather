/*
 * $Id: marshal.c,v 1.6 2009/05/20 19:39:55 tyreld Exp $
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
 * Contributors: Michael Schuele <schuelem@de.ibm.com> 
 *
 * Description: Marshaling
 * 
 * The marshaling functions are dumping their data to a buffer area.
 * Unmarshaling is doing pointer fixups but no relocation.
 */

#include "marshal.h"
#include <string.h>

int marshal_string(const char *str, char *mbuf, off_t *offset, size_t mbuflen,
		   int required)
{
  if (mbuf && offset) {  
    if (required && str == NULL) {
      str = "";
    }
    if (str) {
      if (mbuflen < *offset + strlen(str) + 1) {
	/* length error */
	return -1;
      }
      strcpy(mbuf + *offset,str);
      *offset += strlen(str) + 1;
    }
    return 0;
  }
  /* parameter error */
  return -1;
}

int unmarshal_string(char **str, const char *mbuf, off_t *offset, 
		     size_t mbuflen, int required)
{
  if (str && mbuf && offset) {
    if (*str || required) {
      *str = (char*)mbuf + *offset;
      *offset += strlen(*str) + 1;
      if (*offset > mbuflen) {
	/* length error */
	return -1;
      }
    }
    return 0;
  }
  /* parameter error */
  return -1;
}


int marshal_data(const void *data, size_t datalen, char *mbuf, 
		 off_t *offset, size_t mbuflen)
{
  if (data && mbuf && offset) {  
    if (mbuflen < *offset + datalen) {
      /* length error */
      return -1;
    }
    memcpy(mbuf + *offset, data, datalen);
    *offset += datalen;
    return 0;
  }
  /* parameter error */
  return -1;
}

int unmarshal_data(void **data, size_t datalen, const char *mbuf, 
		   off_t *offset, size_t mbuflen)
{
  if (data && mbuf && offset) {  
    *data = (char*)mbuf + *offset;
    *offset += datalen;
    if (*offset > mbuflen) {
      /* length error */
      return -1;
    }
    return 0;
  }
  /* parameter error */
  return -1;
}

int unmarshal_fixed(void *data, size_t datalen, const char *mbuf, 
		    off_t *offset, size_t mbuflen)
{
  void * datap;
  int rc = unmarshal_data(&datap,datalen,mbuf,offset,mbuflen);
  if (rc==0) {
    memcpy(data,datap,datalen);
  }
  return rc;
}


int marshal_valuerequest(const ValueRequest *vr, char *mbuf, off_t *offset, 
			 size_t mbuflen)
{
  /*
   * strings and other dynamic data is appended after the static parts of the
   * structures. Specifically this means that the buffer will look like
   *  ValueRequest static part
   *    Resource string
   *    SystemId string
   *    ValueItem array static part
   *    ValueItem data dynamic part
   */
  int i;
  if (vr && mbuf && offset) {
    /* process ValueRequest main structure */
    if (marshal_data(vr,sizeof(ValueRequest),mbuf,offset,mbuflen) == -1 || 
	marshal_string(vr->vsResource,mbuf,offset,mbuflen,0) == -1 || 
	marshal_string(vr->vsSystemId,mbuf,offset,mbuflen,0) == -1) {
      return -1;
    }
    /* process ValueItems */
    if (marshal_data(vr->vsValues,sizeof(ValueItem)*vr->vsNumValues,mbuf,
		     offset,mbuflen) == -1 ) { 
      return -1;
    }
    for (i=0;i<vr->vsNumValues;i++) {
      if (marshal_data(vr->vsValues[i].viValue,vr->vsValues[i].viValueLen,
		       mbuf,offset,mbuflen) == -1 ||
	  marshal_string(vr->vsValues[i].viResource,mbuf,offset,mbuflen,0) == -1 || 
	  marshal_string(vr->vsValues[i].viSystemId,mbuf,offset,mbuflen,0) == -1 ) {
	return -1;
      }
    }
    return 0;
  }
  return -1;
}

int unmarshal_valuerequest(ValueRequest **vr, const char *mbuf, off_t *offset, size_t mbuflen)
{
  int i;
  void *vtemp;
  if (vr && mbuf) {
    /* process ValueRequest main structure */
    if (unmarshal_data((void**)vr, sizeof(ValueRequest), mbuf, offset, mbuflen) == -1 ||
	unmarshal_string(&(*vr)->vsResource, mbuf, offset, mbuflen,0) == -1 ||
	unmarshal_string(&(*vr)->vsSystemId, mbuf, offset, mbuflen,0) == -1 ) {
      return -1;
    }
    /* process ValueItems */
    if (unmarshal_data(&vtemp,sizeof(ValueItem)*(*vr)->vsNumValues,mbuf,
		       offset,mbuflen) == -1 ) { 
      return -1;
    }
    (*vr)->vsValues=vtemp;
    for (i=0;i<(*vr)->vsNumValues;i++) {
      if (unmarshal_data(&vtemp,(*vr)->vsValues[i].viValueLen,
			 mbuf,offset,mbuflen) == -1 ||
	  unmarshal_string(&(*vr)->vsValues[i].viResource,mbuf,offset,mbuflen,0) == -1 || 
	  unmarshal_string(&(*vr)->vsValues[i].viSystemId,mbuf,offset,mbuflen,0) == -1 ) {
	return -1;
      }
      (*vr)->vsValues[i].viValue=vtemp;
    }
    return 0;
  }
  return -1;
}



int marshal_subscriptionrequest(const SubscriptionRequest *sr, char *mbuf, 
				off_t *offset, size_t mbuflen)
{
  if (sr && mbuf && offset) {
    if (marshal_data(sr,sizeof(SubscriptionRequest),
		     mbuf,offset,mbuflen) == 0 && 
	marshal_string(sr->srResource,mbuf,offset,mbuflen,0) == 0 &&
	marshal_string(sr->srSystemId,mbuf,offset,mbuflen,0) == 0 &&
	marshal_string(sr->srValue,mbuf,offset,mbuflen,0) == 0) {
      return 0;
    }
  }
  return -1;
}

int unmarshal_subscriptionrequest(SubscriptionRequest **sr, const char *mbuf,
				  off_t *offset, size_t mbuflen)
{
  if (sr && mbuf && offset) {
    if (unmarshal_data((void**)sr,sizeof(SubscriptionRequest),
		       mbuf,offset,mbuflen) == 0 && 
	unmarshal_string(&(*sr)->srResource,mbuf,offset,mbuflen,0) == 0 &&
	unmarshal_string(&(*sr)->srSystemId,mbuf,offset,mbuflen,0) == 0 &&
	unmarshal_string(&(*sr)->srValue,mbuf,offset,mbuflen,0) == 0) {
      return 0;
    }
  }
  return -1;
}

int marshal_reposplugindefinition(const RepositoryPluginDefinition *rdef, 
				  size_t num, char *mbuf,
				  off_t * offset, size_t mbuflen)
{
  int i;
  if (rdef && mbuf && offset) {
    if (marshal_data(rdef,num*sizeof(RepositoryPluginDefinition),
		     mbuf,offset,mbuflen) == 0) {
      for (i=0; i < num; i++) {
	if (marshal_string(rdef[i].rdUnits,mbuf,offset,mbuflen,0) ||
	    marshal_string(rdef[i].rdName,mbuf,offset,mbuflen,0)) {
	  return -1;
	}
      }
      return 0;
    }
  }
  return -1;
}

int unmarshal_reposplugindefinition(RepositoryPluginDefinition **rdef, 
				    size_t num, char *mbuf,
				    off_t * offset, size_t mbuflen)
{
  int i;
  if (rdef && mbuf && offset) {
    if (unmarshal_data((void**)rdef,num*sizeof(RepositoryPluginDefinition),
		       mbuf,offset,mbuflen) == 0) {
      for (i=0; i<num; i++) {
	if (unmarshal_string(&(*rdef)[i].rdUnits,mbuf,offset,mbuflen,0) ||
	    unmarshal_string(&(*rdef)[i].rdName,mbuf,offset,mbuflen,0)) {
	  return -1;
	}
      }
      return 0;
    }
  }
  return -1;
}
