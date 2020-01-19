/*
 * $Id: cimplugOperatingSystem.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * CIM Id Plugin for Operating System specific metrics
 */

#include <cimplug.h>
#include <string.h>
#include <stdlib.h>

CMPIObjectPath* COP4VALID (CMPIBroker *broker, const char *id, 
			const char *systemid)
{
  /* we construct the operating system id according to the OSBase
     rules */
  CMPIObjectPath *cop = CMNewObjectPath(broker,NULL,"Linux_OperatingSystem",
					NULL);
  if (cop) {
    CMAddKey(cop,"Name",systemid,CMPI_chars);
    CMAddKey(cop,"CreationClassName","Linux_OperatingSystem",CMPI_chars);
    CMAddKey(cop,"CSName",systemid,CMPI_chars);
    CMAddKey(cop,"CSCreationClassName","Linux_ComputerSystem",
	     CMPI_chars);
  }
  return cop;
}

int VALID4COP (CMPIObjectPath *cop, char *id, size_t idlen,
	    char *systemid, size_t systemidlen)
{
  CMPIData data;
  char    *str;
  
  if (cop && id && systemid) {
    if (strlen("OperatingSystem")+1 >idlen) {
      return -1;
    } else {
      strcpy(id,"OperatingSystem");
    }
    data=CMGetKey(cop,"CSName",NULL);
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
    *resclasses = calloc(2,sizeof(char*));
    (*resclasses)[0] = strdup("Linux_OperatingSystem");
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
