/*
 * $Id: cimplugNetworkPort.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Heidi Neumann <heidineu@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * CIM Id Plugin for Network Port specific metrics
 */

#include <cimplug.h>
#include <string.h>
#include <stdlib.h>

CMPIObjectPath* COP4VALID (CMPIBroker *broker, const char *id, 
			   const char *systemid)
{
  CMPIObjectPath *cop;
  char *npclass;
  
  if (id==NULL || systemid==NULL) {
    return NULL;
  }

  /* use name to determine port type */
  if (strncmp(id,"eth",3)==0) {
    npclass = "Linux_EthernetPort";
  } else if (strncmp(id,"tr",2)==0) {
    npclass = "Linux_TokenRingPort";
  } else if (strncmp(id,"lo",2)==0) {
    npclass = "Linux_LocalLoopbackPort";
  } else {
    npclass = "CIM_NetworkPort";
  }
  
  cop= CMNewObjectPath(broker,NULL,npclass,NULL);
  if (cop) {
    CMAddKey(cop,"DeviceID",id,CMPI_chars);
    CMAddKey(cop,"CreationClassName",npclass,CMPI_chars);
    CMAddKey(cop,"SystemName",systemid,CMPI_chars);
    CMAddKey(cop,"SystemCreationClassName","Linux_ComputerSystem",
	     CMPI_chars);
  }
  return cop;
}

int VALID4COP (CMPIObjectPath *cop, char *id, size_t idlen,
	       char *systemid, size_t systemidlen)
{
  CMPIData    data;
  CMPIString *clsname;
  char       *str;
  
  if (cop && id && systemid) {
    clsname = CMGetClassName(cop,NULL);
    if (clsname == NULL) {
      return -1;
    }
    str = CMGetCharPtr(clsname);
    if (strcasecmp(str,"Linux_LocalLoopbackPort") && 
	strcasecmp(str,"Linux_Ext3FileSystem") && 
	strcasecmp(str,"Linux_ReiserFileSystem")) {
      return -1;
    }

    data=CMGetKey(cop,"DeviceID",NULL);
    if (data.type==CMPI_string && data.value.string) {
      str=CMGetCharPtr(data.value.string);
      if (strlen(str)<idlen) {
	strcpy(id,str);
      } else {
	return -1;
      }
    }
    data=CMGetKey(cop,"SystemName",NULL);
    if (data.type==CMPI_string && data.value.string) {
      str=CMGetCharPtr(data.value.string);
      if (strlen(str)<systemidlen) {
	strcpy(systemid,str);
	return 0;
      }
    }
  }
  return -1;
}

int GETRES (char *** resclasses)
{
  if (resclasses) {
    *resclasses = calloc(4,sizeof(char*));
    (*resclasses)[0] = strdup("Linux_LocalLoopbackPort");
    (*resclasses)[1] = strdup("Linux_EthernetPort");
    (*resclasses)[2] = strdup("Linux_TokenRingPort");
    return 0;
  }
  return -1;
}

void FREERES (char **resclasses)
{
  int i=0;
  if (resclasses) {
    while (resclasses[i]) {
      free(resclasses[i++]);
    }
    free(resclasses);
  }
}
