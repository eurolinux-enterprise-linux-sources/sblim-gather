/*
 * $Id: cimplugzLPAR.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * CIM plug in for the IBM system z LPAR metrics
 */

#include <cimplug.h>
#include <string.h>
#include <stdlib.h>

CMPIObjectPath* COP4VALID (CMPIBroker *broker, const char *id, 
			   const char *systemid)
{
  CMPIObjectPath *cop = CMNewObjectPath(broker,NULL,"IBMz_ComputerSystem",
					NULL);
  if (cop) {
    CMAddKey(cop,"Name",id,CMPI_chars);
    CMAddKey(cop,"CreationClassName","IBMz_ComputerSystem",CMPI_chars);
  }
  return cop;
}

int VALID4COP (CMPIObjectPath *cop, char *id, size_t idlen,
	       char *systemid, size_t systemidlen)
{
  CMPIData data;
  char    *str;
  
  if (cop && id && systemid) {
    data=CMGetKey(cop,"Name",NULL);
    if (data.type==CMPI_string && data.value.string) {
      str=CMGetCharPtr(data.value.string);
      if (strlen(str)<idlen) {
	strcpy(id,str);
      } else {
	return -1;
      }
    }
  }
  return -1;
}

int GETRES (char *** resclasses)
{
  if (resclasses) {
    *resclasses = calloc(2,sizeof(char*));
    (*resclasses)[0] = strdup("IBMz_ComputerSystem");
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
