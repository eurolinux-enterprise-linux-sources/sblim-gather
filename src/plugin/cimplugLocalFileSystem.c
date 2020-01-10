/*
 * $Id: cimplugLocalFileSystem.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * CIM Id Plugin for Local File System specific metrics
 */

#include <cimplug.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


CMPIObjectPath* COP4VALID (CMPIBroker *broker, const char *id, 
			   const char *systemid)
{
  CMPIObjectPath *cop; 
  char *fsclass;
  char  fsname[300];
  char *fstype, *idx2;
  
  if (id==NULL || systemid==NULL) {
    return NULL;
  }

  strncpy(fsname,id,sizeof(fsname));
  fstype = strchr(fsname,'(');
  idx2 = strchr(fsname,')');
  if (fstype && idx2 && fstype < idx2) {
    *fstype++=0;
    *idx2=0;
  } else {
    return NULL;
  }
  
  if (strcmp(fstype,"ext2")==0) {
    fsclass = "Linux_Ext2FileSystem";
  } else if (strcmp(fstype,"ext3")==0) {
    fsclass = "Linux_Ext3FileSystem";
  } else if (strcmp(fstype,"reiserfs")==0) {
    fsclass = "Linux_ReiserFileSystem";
  } else {
    fsclass = "CIM_UnixLocalFileSystem";
  }
  
  cop = CMNewObjectPath(broker,NULL,fsclass,NULL);
  if (cop) {
    CMAddKey(cop,"Name",fsname,CMPI_chars);
    CMAddKey(cop,"CreationClassName",fsclass,CMPI_chars);
    CMAddKey(cop,"CSName",systemid,CMPI_chars);
    CMAddKey(cop,"CSCreationClassName","Linux_ComputerSystem",
	     CMPI_chars);
    CMAddKey(cop,"OSName",systemid,CMPI_chars);
    CMAddKey(cop,"OSCreationClassName","Linux_OperatingSystem",
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
  char       *fstype;
  
  if (cop && id && systemid) {
    clsname = CMGetClassName(cop,NULL);
    if (clsname == NULL) {
      return -1;
    }
    str = CMGetCharPtr(clsname);
    if (strcasecmp(str,"Linux_Ext2FileSystem")==0) {
      fstype="ext2";
    } else  if (strcasecmp(str,"Linux_Ext3FileSystem")==0) {
      fstype="ext3";
    } else  if (strcasecmp(str,"Linux_ReiserFileSystem")==0) {
      fstype="reiserfs";
    } else {
      fstype="unknown";
    }

    data=CMGetKey(cop,"Name",NULL);
    if (data.type==CMPI_string && data.value.string) {
      str=CMGetCharPtr(data.value.string);
      if (strlen(fstype) + strlen(str) + 2 < idlen) {
	sprintf(id,"%s(%s)",str,fstype);
      } else {
	return -1;
      }
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
    *resclasses = calloc(4,sizeof(char*));
    (*resclasses)[0] = strdup("Linux_Ext2FileSystem");
    (*resclasses)[1] = strdup("Linux_Ext3FileSystem");
    (*resclasses)[2] = strdup("Linux_ReiserFileSystem");
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
