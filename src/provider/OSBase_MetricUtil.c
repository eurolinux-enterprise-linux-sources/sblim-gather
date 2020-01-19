/*
 * $Id: OSBase_MetricUtil.c,v 1.24 2012/10/25 23:13:52 tyreld Exp $
 *
 * © Copyright IBM Corp. 2004, 2007, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:      Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Description: Utility Functions for Metric Providers
 * 
 */

#include "OSBase_MetricUtil.h"
#include <OSBase_Common.h>
#include <string.h>
#include <stdio.h>
#include <rrepos.h>
#include <mrwlock.h>
#include <cmpift.h>
#include <cmpimacs.h>
#include <cimplug.h>
#include <dlfcn.h>


#ifndef CIM_PLUGINDIR
#define CIM_PLUGINDIR "/usr/lib/gather/cplug"
#endif

#define PLUGINCLASSNAME "Linux_RepositoryPlugin"
#define VALDEFCLASSNAME "Linux_MetricValueDefinition"

static unsigned char CMPI_false = 0;
static unsigned char CMPI_true  = 1;

int checkRepositoryConnection()
{
  RepositoryStatus stat;
  if (rrepos_status(&stat)==0 && stat.rsInitialized)
    return 1;
  else
    return 0;
}

MRWLOCK_DEFINE(MdefLock);
MRWLOCK_DEFINE(PdefLock);

static struct _MdefList {
  char * mdef_metricname;
  int    mdef_metricid;
  char * mdef_classname;
  char * mdef_pluginname;
  char * mdef_cimpluginname;
  int    mdef_datatype;
  int    mdef_metrictype;
  int    mdef_changetype;
  int    mdef_iscontinuous;
  unsigned short mdef_calculable;
  char * mdef_units;
} * metricDefinitionList = NULL;

static struct _MvalList {
  char * mval_classname;
  char * mdef_classname;
} * metricValueList = NULL;

static struct _PluginList {
  char                     * plug_name;
  void                     * plug_handle;
  CimObjectPathFromValueId * plug_cop4id;
  ValueIdFromCimObjectPath * plug_id4cop;
  GetResourceClasses       * plug_getres;
  FreeResourceClasses      * plug_freeres;
} * pluginList = NULL;

/*
 * refresh the cache of metric definition classes
 */

static void removePluginList();

static void removeValueList()
{
  int i;
  /* assume lock is already done */
  if (metricValueList) {
    i=0;
    while(metricValueList[i].mval_classname) {      
      free(metricValueList[i].mval_classname);
      free(metricValueList[i].mdef_classname);
      i++;
    }
    free(metricValueList);
    metricValueList = NULL;
  }
  
}

static void removeDefinitionList()
{
  int i;
  /* assume lock is already done */
  if (metricDefinitionList) {
    /* free string pointers */
    i=0;
    while(metricDefinitionList[i].mdef_metricname) {
      free(metricDefinitionList[i].mdef_metricname);
      free(metricDefinitionList[i].mdef_classname);
      free(metricDefinitionList[i].mdef_pluginname);
      free(metricDefinitionList[i].mdef_units);
      free(metricDefinitionList[i].mdef_cimpluginname);
      i++;
    }
    free(metricDefinitionList);
    removePluginList();
    metricDefinitionList = NULL;
  }
}

static int refreshMetricValueList(const CMPIBroker * broker, const CMPIContext * ctx, 
			      const char *namesp)
{
  CMPIData         data, defdata, valdata;
  CMPIObjectPath  *co = CMNewObjectPath(broker,namesp,VALDEFCLASSNAME,NULL);
  CMPIEnumeration *en = CBEnumInstances(broker,ctx,co,NULL,NULL);
  int              totalnum = 0;

  /* assume lock is already done */
  _OSBASE_TRACE(4,("refreshMetricValueList() - namespace %s\n",namesp));
  removeValueList();
  while (en && CMHasNext(en,NULL)) {
    data = CMGetNext(en,NULL);
    if (data.value.inst) {
      valdata=CMGetProperty(data.value.inst,"MetricValueClassName",NULL);
      defdata=CMGetProperty(data.value.inst,"MetricDefinitionClassName",
			    NULL);
    }
    if (valdata.value.string && defdata.value.string) {
      metricValueList = realloc(metricValueList,
				sizeof(struct _MvalList)*(totalnum+2));
      metricValueList[totalnum].mval_classname = 
	strdup(CMGetCharPtr(valdata.value.string));
      metricValueList[totalnum].mdef_classname = 
	strdup(CMGetCharPtr(defdata.value.string));
      totalnum++;
      metricValueList[totalnum].mval_classname = NULL; /* End of list */
    } else {
      return -1;
    }
  }
  return 0;
}

int refreshMetricDefClasses(const CMPIBroker * broker, const CMPIContext * ctx, 
			    const char *namesp, int force)
     /* TODO: rework error handling */
{
  CMPIData         data, plugindata, cimplugdata;
  CMPIObjectPath  *co;
  CMPIEnumeration *en;
  char            *pname, *cpname;
  int              rdefnum, i, totalnum;
  RepositoryPluginDefinition *rdef;
  COMMHEAP         ch;

  _OSBASE_TRACE(4,("refreshMetricDefClasses() - namespace %s\n",namesp));
  /* refresh definition/value mapping list */
  MWriteLock(&MdefLock);
  if ((force || metricDefinitionList == NULL) && refreshMetricValueList(broker,ctx,namesp)) {
    MWriteUnlock(&MdefLock);
    return -1;
  }

  /* check for repository connection */
  if (checkRepositoryConnection()==0) {
    MWriteUnlock(&MdefLock);
    return -1;
  }

  if (!force && metricDefinitionList) {
    MWriteUnlock(&MdefLock);
    return 0;
  }
  co = CMNewObjectPath(broker,namesp,PLUGINCLASSNAME,NULL);
  en = CBEnumInstances(broker,ctx,co,NULL,NULL);
  ch=ch_init();
  removeDefinitionList();
  /* loop over all repository plugins to get to the metric id to
     classname mappings */
  totalnum=0;
  while (en && CMHasNext(en,NULL)) {
    data = CMGetNext(en,NULL);
    if (data.value.inst) {
      plugindata=CMGetProperty(data.value.inst,"RepositoryPluginName",NULL);
      cimplugdata=CMGetProperty(data.value.inst,"CIMTranslationPluginName",
				NULL);
      data = CMGetProperty(data.value.inst,"MetricDefinitionClassName",NULL);
      if (plugindata.value.string && data.value.string  && 
	  cimplugdata.value.string) {	
	pname = CMGetCharPtr(plugindata.value.string);
	cpname = CMGetCharPtr(cimplugdata.value.string);
	rdefnum = rreposplugin_list(pname,&rdef,ch);
	if (rdefnum < 0) {
	  continue;
	}
	metricDefinitionList = realloc(metricDefinitionList,
				       (rdefnum+totalnum+1)*sizeof(struct _MdefList));
	for (i=0;i<rdefnum;i++) {
	  metricDefinitionList[totalnum+i].mdef_metricname = strdup(rdef[i].rdName);
	  metricDefinitionList[totalnum+i].mdef_metricid = rdef[i].rdId;
	  metricDefinitionList[totalnum+i].mdef_classname = 
	    strdup(CMGetCharPtr(data.value.string));
	  metricDefinitionList[totalnum+i].mdef_pluginname = strdup(pname);
	  metricDefinitionList[totalnum+i].mdef_cimpluginname = strdup(cpname);
	  metricDefinitionList[totalnum+i].mdef_datatype = rdef[i].rdDataType;
	  metricDefinitionList[totalnum+i].mdef_metrictype = rdef[i].rdMetricType;
	  metricDefinitionList[totalnum+i].mdef_changetype = rdef[i].rdChangeType;
	  metricDefinitionList[totalnum+i].mdef_iscontinuous = rdef[i].rdIsContinuous;
	  metricDefinitionList[totalnum+i].mdef_calculable = rdef[i].rdCalculable;
	  metricDefinitionList[totalnum+i].mdef_units = strdup(rdef[i].rdUnits?rdef[i].rdUnits:"n/a");
	}
	/* identify last element with null name */
	totalnum += rdefnum;
	metricDefinitionList[totalnum].mdef_metricname = NULL;
      }
    }
  }
  MWriteUnlock(&MdefLock);
  ch_release(ch);
  return 0;
}

void releaseMetricDefClasses()
{
  _OSBASE_TRACE(4,("releaseMetricDefClasses()\n"));
  MWriteLock(&MdefLock);
  if (metricDefinitionList) {
    removeDefinitionList();
  }
  if (metricValueList) {
    removeValueList();
  }
  MWriteUnlock(&MdefLock);
}

static int metricDefClassIndex(const CMPIBroker *broker, const CMPIContext *ctx, 
		       const char *namesp,const char *name, int id)
{
  int i=0;
  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while (metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (strcmp(name,metricDefinitionList[i].mdef_metricname) == 0 &&
	id == metricDefinitionList[i].mdef_metricid) {
      MReadUnlock(&MdefLock);
      return i;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return -1;
}

int metricDefClassName(const CMPIBroker *broker, const CMPIContext *ctx, 
		       const char *namesp,char *clsname, 
		       const char *name, int id)
{
  int i=0;
  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while (metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (strcmp(name,metricDefinitionList[i].mdef_metricname) == 0 &&
	id == metricDefinitionList[i].mdef_metricid) {
      strcpy(clsname,metricDefinitionList[i].mdef_classname);
      MReadUnlock(&MdefLock);
      return 0;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return -1;
}

int metricPluginName(const CMPIBroker *broker, const CMPIContext *ctx, 
		     const char *namesp,char *pluginname, 
		     const char *name, int id)
{
  int i=0;
  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while (metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (strcmp(name,metricDefinitionList[i].mdef_metricname) == 0 &&
	id == metricDefinitionList[i].mdef_metricid) {
      strcpy(pluginname,metricDefinitionList[i].mdef_pluginname);
      MReadUnlock(&MdefLock);
      return 0;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return -1;
}

int metricValueClassName(const CMPIBroker *broker, const CMPIContext *ctx, 
			 const char *namesp,char *clsname, 
			 const char *name, int id)
{
  int  i=0;
  char defname[500];
  
  if (metricDefClassName(broker,ctx,namesp,defname,name,id)) {
    return -1;
  }
  
  MReadLock(&MdefLock);
  while (metricValueList && metricValueList[i].mval_classname) {
    if (strcasecmp(defname,metricValueList[i].mdef_classname)==0){
      strcpy(clsname,metricValueList[i].mval_classname);
      MReadUnlock(&MdefLock);
      return 0;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return -1;
}

char * makeMetricDefIdFromCache(const CMPIBroker *broker, const CMPIContext *ctx,
				const char *namesp, char * defid, int id)
{
  int i=0;
  char name[1000];
  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while(metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (metricDefinitionList[i].mdef_metricid==id) {
      strcpy(name,metricDefinitionList[i].mdef_metricname);
      MReadUnlock(&MdefLock);
      return makeMetricDefId(defid,name,id);
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return NULL;
}

char * makeMetricValueIdFromCache(const CMPIBroker *broker, const CMPIContext *ctx,
				  const char * namesp, char * valid, int id,
				  const char * resource, const char * systemid,
				  time_t timestamp)
{
  int i=0;
  char name[1000];
  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while(metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (metricDefinitionList[i].mdef_metricid==id) {
      strcpy(name,metricDefinitionList[i].mdef_metricname);
      MReadUnlock(&MdefLock);
      return makeMetricValueId(valid,name,id,resource,systemid,timestamp);
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return NULL;
}

char * makeMetricDefId(char * defid, const char * name, int id)
{
  char *xname = NULL, *tmp;
  int   xidx;

  if (name==NULL) {
    return NULL;
  }

  if (strchr(name,'.')) {
    /* escaping single dots by doubling them */
    xname = malloc(2*strlen(name)+1);
    xidx = 0;
    while ((tmp=strchr(name,'.'))) {
      memcpy(xname + xidx, name, (tmp-name));
      xidx += (tmp-name) + 2;
      xname[xidx-2]='.';
      xname[xidx-1]='.';
      name = tmp + 1;      
    }
    strcpy(xname+xidx,name);
    name=xname;
  }
  sprintf(defid,"%s.%d",name,id);
  if (xname) {
    free (xname);
  }
  return defid;
}    
  
int parseMetricDefId(const char * defid,
		     char * name, int * id)
{
  char *parsebuf = defid ? strdup(defid) : NULL;
  char *nextdf = parsebuf ? strstr(parsebuf,"..") : NULL;
  char *nextf = parsebuf ? strchr(parsebuf,'.') : NULL;
  while (nextdf && nextf == nextdf) {
    memmove(nextdf+1,nextdf+2,strlen(nextdf+2)+1); /* reduce double dot */
    nextf++;
    nextdf = strstr(nextf,"..");
    nextf = strchr(nextf,'.');
  }
  if (nextf) {
    *nextf=0;
    strcpy(name,parsebuf);
    sscanf(nextf+1,"%d",id);
    if (parsebuf) {
      free(parsebuf);
    }
    return 0;
  } else {
    if (parsebuf) {
      free(parsebuf);
    }
    return -1;
  }
}

char * makeMetricValueId(char * instid, const char * name, int id,  
			 const char * resource, const char * systemid,
			 time_t timestamp)
{
  char *xname=NULL;
  char *xresource=NULL;
  char *xsystemid=NULL;
  char *tmp;
  int   xidx;

  if (name==NULL || resource==NULL || systemid==NULL) {
    return NULL;
  }
  
  if (strchr(name,'.')) {
    /* escaping single dots by doubling them */
    xname = malloc(2*strlen(name)+1);
    xidx = 0;
    while ((tmp=strchr(name,'.'))) {
      memcpy(xname + xidx, name, (tmp-name));
      xidx += (tmp-name) + 2;
      xname[xidx-2]='.';
      xname[xidx-1]='.';
      name = tmp + 1;      
    }
    strcpy(xname+xidx,name);
    name=xname;
  }
  if (strchr(resource,'.')) {
    /* escaping single dots by doubling them */
    xresource = malloc(2*strlen(resource)+1);
    xidx = 0;
    while ((tmp=strchr(resource,'.'))) {
      memcpy(xresource + xidx, resource, (tmp-resource));
      xidx += (tmp-resource) + 2;
      xresource[xidx-2]='.';
      xresource[xidx-1]='.';
      resource = tmp + 1;      
    }
    strcpy(xresource+xidx,resource);
    resource=xresource;
  }
  if (strchr(systemid,'.')) {
    /* escaping single dots by doubling them */
    xsystemid = malloc(2*strlen(systemid)+1);
    xidx = 0;
    while ((tmp=strchr(systemid,'.'))) {
      memcpy(xsystemid + xidx, systemid, (tmp-systemid));
      xidx += (tmp-systemid) + 2;
      xsystemid[xidx-2]='.';
      xsystemid[xidx-1]='.';
      systemid = tmp + 1;      
    }
    strcpy(xsystemid+xidx,systemid);
    systemid=xsystemid;
  }
  sprintf(instid,"%s.%d.%s.%s.%ld",name,id,resource,systemid,timestamp);
  if (xname) {
    free(xname);
  }
  if (xresource) {
    free(xresource);
  }
  if (xsystemid) {
    free(xsystemid);
  }
  return instid;
}


#define MVNUMELEMENTS 5

int parseMetricValueId(const char * instid,
		       char * name, int * id, char * resource,
		       char * systemid, time_t * timestamp)
{
  char *parsebuf = instid ? strdup(instid) : NULL;
  char *nextdf = parsebuf ? strstr(parsebuf,"..") : NULL;
  char *nextf = parsebuf ? strchr(parsebuf,'.') : NULL;
  char *idxptr[MVNUMELEMENTS] = {parsebuf,NULL,NULL,NULL,NULL};
  int   i=1;

  /* get seperator positions */
  while (nextf && i < MVNUMELEMENTS) {
    while (nextdf && nextf == nextdf) {
      memmove(nextdf+1,nextdf+2,strlen(nextdf+2)+1); /* reduce double dot */
      nextf++;
      nextdf = strstr(nextf,"..");
      nextf = strchr(nextf,'.');
    }
    *nextf=0;
    nextf++;
    idxptr[i++]=nextf;
    nextdf = strstr(nextf,"..");
    nextf = strchr(nextf,'.');
  }
  /* fill out result parameters */
  if (i == MVNUMELEMENTS) {
    strcpy(name,idxptr[0]);
    sscanf(idxptr[1],"%d",id);
    strcpy(resource,idxptr[2]);
    strcpy(systemid,idxptr[3]);
    sscanf(idxptr[4],"%ld",timestamp);
    if (parsebuf) {
      free(parsebuf);
    }
    return 0;
  } else {
    if (parsebuf) {
      free(parsebuf);
    }
    return -1;
  }
}

static char * metricClassName(const CMPIObjectPath *path)
{
  /* return class name or NULL if CIM_ (base) class */
  CMPIStatus rc;
  CMPIString * clsname = CMGetClassName(path,&rc);

  if (clsname) {
    return CMGetCharPtr(clsname);
  } else {
    return NULL;
  }
}

static int pluginForClass(const CMPIBroker *broker, const CMPIContext *ctx, 
			  const CMPIObjectPath *cop, char *pluginname)
{
  char *metricclass = metricClassName(cop);
  int   i=0;

  refreshMetricDefClasses(broker,ctx,CMGetCharPtr(CMGetNameSpace(cop,NULL)),0);

  MReadLock(&MdefLock);
  while(metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    if (strcasecmp(metricclass,metricDefinitionList[i].mdef_classname)==0) {
      strcpy(pluginname,metricDefinitionList[i].mdef_pluginname);
      MReadUnlock(&MdefLock);
      return 0;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return -1;
}

int getPluginNamesForValueClass(const CMPIBroker *broker, const CMPIContext *ctx, 
				const CMPIObjectPath *cop, char ***pluginnames)
{
  char  pluginname[500];
  char *valclassname;
  int   i=0, j=0;
  int   totalnum=0;
 
  if (pluginnames==NULL) {
    return -1;
  }
  
  /* search for matching value class and get plugin if found */
  refreshMetricDefClasses(broker,ctx,CMGetCharPtr(CMGetNameSpace(cop,NULL)),0);
  MReadLock(&MdefLock);
  valclassname=CMGetCharPtr(CMGetClassName(cop,NULL));
  pluginname[0]=0;
  while(metricValueList && metricValueList[i].mval_classname) {
    if (strcasecmp(valclassname,metricValueList[i].mval_classname)==0) {
      /* found definition class for valu class - now get plugin */
      while(metricDefinitionList && metricDefinitionList[j].mdef_metricname) {
	if (strcasecmp(metricValueList[i].mdef_classname,
		   metricDefinitionList[j].mdef_classname)==0) {
	  strcpy(pluginname, metricDefinitionList[j].mdef_pluginname);
	  break;
	}
	j++;
      }
      break;
    }
    i++;
  }
  MReadUnlock(&MdefLock);
	
  if (pluginname[0]){
    /* found plugin for this class */
    *pluginnames = malloc(2*sizeof(char*));
    (*pluginnames)[0] = strdup(pluginname);
    (*pluginnames)[1] = NULL;
    return 1;
  } else if (strncasecmp("CIM_",valclassname,strlen("CIM_"))==0) {
    /* no exact match - return all plugins */
    *pluginnames=NULL;
    MReadLock(&MdefLock);
    i=0;
    while(metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
      if (strcmp(pluginname,metricDefinitionList[i].mdef_pluginname)) {
	/* a new plugin name found -- add to list */
	strcpy(pluginname,metricDefinitionList[i].mdef_pluginname);
	*pluginnames=realloc(*pluginnames, sizeof(char*)*(totalnum+2));
	(*pluginnames)[totalnum]=strdup(pluginname);
	(*pluginnames)[totalnum+1]=NULL;
	totalnum++;
      }
      i++;
    }
    MReadUnlock(&MdefLock);    
    return totalnum;
  } else {
    *pluginnames=NULL;
    return 0;
  }
}

void releasePluginNames(char **pluginnames)
{
  int i = 0;
  if (pluginnames) {
    while (pluginnames[i]) {
      free (pluginnames[i]);
      i++;
    }
    free (pluginnames);
  }
}

int getMetricDefsForClass(const CMPIBroker *broker, const CMPIContext *ctx, 
			  const CMPIObjectPath* cop,
			  char ***mnames, int **mids)
{
  char pluginname[500];
  int  i=0;
  int  totalnum=0;
  
  if (mnames && mids) {
    *mnames=NULL;
    *mids=NULL;
    if (pluginForClass(broker,ctx,cop,pluginname)==0) {
      /* search for given plugin name */
      MReadLock(&MdefLock);
      while (metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
	if (strcmp(metricDefinitionList[i].mdef_pluginname,pluginname)==0) {
	  *mnames = realloc(*mnames,(totalnum+2)*sizeof(char*));
	  *mids = realloc(*mids,(totalnum+1)*sizeof(int));
	  (*mnames)[totalnum] = strdup(metricDefinitionList[i].mdef_metricname);
	  (*mnames)[totalnum+1] = NULL;
	  (*mids)[totalnum] = metricDefinitionList[i].mdef_metricid;
	  totalnum++;
	}
	i++;
      }
      MReadUnlock(&MdefLock);
    } else if (strncasecmp("CIM_",metricClassName(cop),strlen("CIM_"))==0) {
      /* return all - should only happen for polymorphic call in SNIA CIMOM */
      MReadLock(&MdefLock);
      while (metricDefinitionList && 
	     metricDefinitionList[totalnum].mdef_metricname) {
	*mnames = realloc(*mnames,(totalnum+2)*sizeof(char*));
	*mids = realloc(*mids,(totalnum+1)*sizeof(int));
	(*mnames)[totalnum] = 
	  strdup(metricDefinitionList[totalnum].mdef_metricname);
	(*mnames)[totalnum+1] = NULL;
	(*mids)[totalnum] = metricDefinitionList[totalnum].mdef_metricid;
	totalnum++;
      }
      MReadUnlock(&MdefLock);     
    }
  }
  return totalnum;
}

void releaseMetricDefs(char **mnames,int *mids)
{
  int i=0;
  if (mnames) {
    while(mnames[i]) {
      free(mnames[i]);
      i++;
    }
    free(mnames);
  }
  if (mids) {
    free(mids);
  }
}

static CimObjectPathFromValueId * _GetCop4Id(const char *pluginname);
static ValueIdFromCimObjectPath * _GetId4Cop(const char *pluginname);
static GetResourceClasses * _GetGetRes(const char *pluginname);
static FreeResourceClasses * _GetFreeRes(const char *pluginname);

int getMetricIdsForResourceClass(const CMPIBroker *broker, const CMPIContext *ctx, 
				 const CMPIObjectPath* cop,
				 char ***metricnames,
				 int **mids, 
				 char ***resourceids,
				 char ***systemids)
{
  /* inefficient -- need better buffering */
  GetResourceClasses       *getres;
  FreeResourceClasses      *freeres;
  ValueIdFromCimObjectPath *id4cop;
  char                    **resclasses;
  CMPIObjectPath           *cores;
  int                       i=0, j, k=0;
  char                     *classname;
  char                     *namesp;
  char                      resourcename[300];
  char                      systemname[300];


  *mids=NULL;
  *resourceids=NULL;
  *systemids=NULL;
  *metricnames=NULL;
  classname=CMGetCharPtr(CMGetClassName(cop,NULL));
  namesp=CMGetCharPtr(CMGetNameSpace(cop,NULL));

  refreshMetricDefClasses(broker,ctx,namesp,0);

  MReadLock(&MdefLock);
  while (metricDefinitionList && metricDefinitionList[i].mdef_metricname) {
    getres=_GetGetRes(metricDefinitionList[i].mdef_cimpluginname);
    freeres=_GetFreeRes(metricDefinitionList[i].mdef_cimpluginname);
    if (getres && freeres) {
      j=0;
      getres(&resclasses);
      while (resclasses[j]) {
	cores=CMNewObjectPath(broker,namesp,resclasses[j],NULL);
	if (cores) {
	  if (CMClassPathIsA(broker,cores,classname,NULL)) {
	    /* this plugin is responsible for resource class */
	    id4cop=_GetId4Cop(metricDefinitionList[i].mdef_cimpluginname);
	    if (id4cop) {
	      if (id4cop(cop,resourcename,
			 sizeof(resourcename),systemname,sizeof(systemname))==0) {
		*mids=realloc(*mids,sizeof(int)*(k+1));
		*metricnames=realloc(*metricnames,sizeof(char*)*(k+2));
		*resourceids=realloc(*resourceids,sizeof(char*)*(k+1));
		*systemids=realloc(*systemids,sizeof(char*)*(k+1));
		(*metricnames)[k] = 
		  strdup(metricDefinitionList[i].mdef_metricname);
		(*metricnames)[k+1] = NULL;
		(*mids)[k] = metricDefinitionList[i].mdef_metricid;
		(*resourceids)[k] = strdup(resourcename);
		(*systemids)[k] = strdup(systemname);
		k++;
	      }
	    }
	    break;
	  }
	}
	j++;
      }
      freeres(resclasses);
    }
    i++;
  }
  MReadUnlock(&MdefLock);
  return k;
}

void releaseMetricIds(char **metricnames,int *mids, char **resourceids,
		      char **systemids)  
{
}

CMPIString * val2string(const CMPIBroker * broker, const ValueItem *val,
			unsigned   datatype)
{
  char valbuf[1000];
  switch (datatype) {
  case MD_BOOL:
    sprintf(valbuf,"%s", *val->viValue ? "true" : "false" );
    break;
  case MD_UINT8:
    sprintf(valbuf,"%hhu",*(unsigned short*)val->viValue);
    break;
  case MD_SINT8:
    sprintf(valbuf,"%hhd",*(short*)val->viValue);
    break;
  case MD_UINT16:
  case MD_CHAR16:
    sprintf(valbuf,"%hu",*(unsigned short*)val->viValue);
    break;
  case MD_SINT16:
    sprintf(valbuf,"%hd",*(short*)val->viValue);
    break;
  case MD_UINT32:
    sprintf(valbuf,"%u",*(unsigned*)val->viValue);
    break;
  case MD_SINT32:
    sprintf(valbuf,"%d",*(int*)val->viValue);
    break;
  case MD_UINT64:
    sprintf(valbuf,"%llu",*(unsigned long long*)val->viValue);
    break;
  case MD_SINT64:
    sprintf(valbuf,"%lld",*(long long*)val->viValue);
    break;
  case MD_FLOAT32:
    sprintf(valbuf,"%f",*(float*)val->viValue);
    break;
  case MD_FLOAT64:
    sprintf(valbuf,"%f",*(double*)val->viValue);
    break;
  case MD_STRING:
    strcpy(valbuf,val->viValue);
    break;
  default:
    sprintf(valbuf,"datatype %0x not supported",datatype);
    break;
  }
  return CMNewString(broker,valbuf,NULL);
}

CMPIInstance * makeMetricValueInst(const CMPIBroker * broker, 
				   const CMPIContext * ctx,
				   const char * defname,
				   int    defid,
				   const ValueItem *val, 
				   unsigned   datatype,
				   const CMPIObjectPath *cop,
                                   const char ** props,
				   CMPIStatus * rc)
{
  CMPIObjectPath * co;
  CMPIInstance   * ci = NULL;
  char             instid[1000];
  char             defidstr[1000];
  char             valclsname[1000];
  char           * namesp;
  CMPIDateTime   * datetime;
  CMPIString     * valstring;

  namesp=CMGetCharPtr(CMGetNameSpace(cop,NULL));
  if (metricValueClassName(broker,ctx,namesp,valclsname,defname,defid)) {
    return NULL;
  }
  
  co = CMNewObjectPath(broker,namesp,valclsname,rc);
  if (co) {
    ci = CMNewInstance(broker,co,rc);

    if (ci) {  
      CMSetPropertyFilter(ci, props, NULL);

    CMSetProperty(ci,"InstanceId",
        makeMetricValueId(instid,defname,defid,val->viResource,
	                val->viSystemId,
			val->viCaptureTime),
		        CMPI_chars);
    CMSetProperty(ci,"MetricDefinitionId",makeMetricDefId(defidstr,defname,defid),
                        CMPI_chars);
      CMSetProperty(ci,"MeasuredElementName",val->viResource,CMPI_chars);
      datetime = 
	CMNewDateTimeFromBinary(broker,
				(long long)val->viCaptureTime*1000000,
				0, NULL);
      if (datetime)
	CMSetProperty(ci,"TimeStamp",&datetime,CMPI_dateTime);
      datetime = 
	CMNewDateTimeFromBinary(broker,
				(long long)val->viDuration*1000000,
				1, NULL);
      if (datetime)
	CMSetProperty(ci,"Duration",&datetime,CMPI_dateTime);

      valstring = val2string(broker,val,datatype);
      if (valstring)
	CMSetProperty(ci,"MetricValue",&valstring,CMPI_string);

      CMSetProperty(ci,"Volatile",&CMPI_false,CMPI_boolean);
    }
  }
  return ci;
}

CMPIObjectPath * makeMetricValuePath(const CMPIBroker * broker,
				     const CMPIContext * ctx,
				     const char * defname,
				     int    defid,
				     const ValueItem *val, 
				     const CMPIObjectPath *cop,
				     CMPIStatus * rc)
{
  CMPIObjectPath * co;
  char           * namesp;
  char             instid[1000];
  char             valclsname[1000];

  namesp=CMGetCharPtr(CMGetNameSpace(cop,NULL));
  if (metricValueClassName(broker,ctx,namesp,valclsname,defname,defid)) {
    return NULL;
  }
  co = CMNewObjectPath(broker,namesp,valclsname,rc);
  if (co) {
    CMAddKey(co,"InstanceId",
	     makeMetricValueId(instid,defname,defid,val->viResource,
			       val->viSystemId,val->viCaptureTime),
	     CMPI_chars);
    /* TODO: need to add more data (for id, etc.) to value items */
    CMAddKey(co,"MetricDefinitionId",makeMetricDefId(instid,defname,defid),
	     CMPI_chars);
  }
  return co;
}

CMPIObjectPath * makeMetricDefPath(const CMPIBroker * broker,
				   const CMPIContext * ctx,
				   const char * defname,
				   int    defid,
				   const char *namesp,
				   CMPIStatus * rc)
{
  CMPIObjectPath * co;
  char             instid[1000];
  char             defclsname[1000];

  if (metricDefClassName(broker,ctx,namesp,defclsname,defname,defid)) {
    return NULL;
  }
  co = CMNewObjectPath(broker,namesp,defclsname,rc);
  if (co) {
    CMAddKey(co,"Id",
	     makeMetricDefId(instid,defname,defid),
	     CMPI_chars);
  }
  return co;
}


static int metrictypetable[] = {
  -1,
  MD_POINT,
  MD_INTERVAL,
  MD_STARTUPINTERVAL,
  MD_RATE,
  MD_AVERAGE,
};

static int mapmetrictypetable[] = {
  0,
  2,
  3,
  4,
  3,
  3,
};

static int changetypetable[] = {
  -1,
  -1,
  -1,
  MD_COUNTER,
  MD_GAUGE,
};

static int typetable[] = {
  -1,
  MD_BOOL,
  MD_CHAR16,
  -1,
  MD_FLOAT32,
  MD_FLOAT64,
  MD_SINT16,
  MD_SINT32,
  MD_SINT64,
  MD_SINT8,
  MD_STRING,
  MD_UINT16,
  MD_UINT32,
  MD_UINT64,
  MD_UINT8,
};

CMPIInstance * makeMetricDefInst(const CMPIBroker * broker,
				 const CMPIContext * ctx,
				 const char * defname,
				 int    defid,
				 const char *namesp,
                                 const char **props,
				 CMPIStatus * rc)
{
  CMPIObjectPath * co;
  CMPIInstance   * ci = NULL;
  char             instid[500];
  size_t           dt;
  int              i;

  i = metricDefClassIndex(broker,ctx,namesp,defname,defid);
  if (i < 0) {
    return NULL;
  }
  co = CMNewObjectPath(broker,namesp,metricDefinitionList[i].mdef_classname,rc);
  
  /* get plugin's metric definition */
  if (co) {
    ci = CMNewInstance(broker,co,rc);
    if (ci) {
      CMSetPropertyFilter(ci, props, NULL);

      CMSetProperty(ci,"Id",
              makeMetricDefId(instid,defname,defid),CMPI_chars);

      if (metricDefinitionList[i].mdef_metrictype&MD_ORGSBLIM) {
        sprintf(instid,"SBLIM:%s",defname);
        CMSetProperty(ci,"Name",instid,CMPI_chars);
      } else {
        CMSetProperty(ci,"Name",defname,CMPI_chars);
      }
      /* DataType */
      for (dt=0;dt<sizeof(typetable)/sizeof(int);dt++) {
	if (metricDefinitionList[i].mdef_datatype==typetable[dt])
	  break;
      }
      if (dt<sizeof(typetable)/sizeof(int))
	CMSetProperty(ci,"DataType",&dt,CMPI_uint16);
      /* GatheringType */
      dt=3;
      if (metricDefinitionList[i].mdef_metrictype&MD_PERIODIC)
	CMSetProperty(ci,"GatheringType",&dt,CMPI_uint16);
      /* TimeScope */
      for (dt=0;dt<sizeof(metrictypetable)/sizeof(int);dt++) {
	if ((metricDefinitionList[i].mdef_metrictype&metrictypetable[dt])
	    ==metrictypetable[dt])
	  break;
      }
      if (dt<sizeof(mapmetrictypetable)/sizeof(int))
	CMSetProperty(ci,"TimeScope",&mapmetrictypetable[dt],CMPI_uint16);
      else 
	CMSetProperty(ci,"TimeScope",&mapmetrictypetable[0],CMPI_uint16);
      /* IsContinuous, ChangeType */
      if (metricDefinitionList[i].mdef_iscontinuous&MD_TRUE) {
	CMSetProperty(ci,"IsContinuous",&CMPI_true,CMPI_boolean);
	for (dt=0;dt<sizeof(changetypetable)/sizeof(int);dt++) {
	  if (metricDefinitionList[i].mdef_changetype==changetypetable[dt])
	    break;
	}
	if (dt<sizeof(changetypetable)/sizeof(int))
	  CMSetProperty(ci,"ChangeType",&dt,CMPI_uint16);
	else {
	  dt=0;
	  CMSetProperty(ci,"ChangeType",&dt,CMPI_uint16);
	}
      }
      else {
	CMSetProperty(ci,"IsContinuous",&CMPI_false,CMPI_boolean);
	dt=2;
	CMSetProperty(ci,"ChangeType",&dt,CMPI_uint16);
      }
      /* Calculable */
      CMSetProperty(ci, "Calculable", 
		    &metricDefinitionList[i].mdef_calculable, 
		    CMPI_uint16);
      /* Units */
      CMSetProperty(ci, "Units", metricDefinitionList[i].mdef_units, 
		    CMPI_chars);
      return ci;
    }
  }
  return NULL;
}


CMPIObjectPath * makeResourcePath(const CMPIBroker * broker,
				  const CMPIContext * ctx,
				  const char * namesp,
				  const char * defname,
				  int defid,
				  const char * resourcename,
				  const char * systemid)
{
  CimObjectPathFromValueId * cop4id;
  int idx=metricDefClassIndex(broker,ctx,namesp,defname,defid);
  if (idx >= 0 && metricDefinitionList[idx].mdef_cimpluginname) {
    cop4id = _GetCop4Id(metricDefinitionList[idx].mdef_cimpluginname);
    if (cop4id) {
      return (*cop4id)(broker,resourcename,systemid);
    }
  }
  return NULL;
}

/* adjust namespace for the resource object path.
 * this is the default implementation that uses the systemid
 * to construct to compute a namespace for a remote resource if needed.
 * NOTE: For special purposes, i.e. remote namespaces with remote CMPI
 *       under Pegasus, it will be necessary to write alternative
 *       namespace computing functions and load them via a plugin mechanism.
 */ 
void computeResourceNamespace(const CMPIObjectPath *rescop,
			      const CMPIObjectPath *mcop,
			      const char *systemid)
{
  char * localnsp = CMGetCharPtr(CMGetNameSpace(mcop,NULL));
  char * resnsp = CMGetCharPtr(CMGetNameSpace(rescop,NULL));

  if (!resnsp && localnsp) {
    CMSetNameSpace((CMPIObjectPath*)rescop,localnsp);
    CMSetHostname((CMPIObjectPath*)rescop,systemid);
  }
}

static void removePluginList()
{
  int i;
  /* assume lock is already done */
  if (pluginList) {
    i=0;
    while(pluginList[i].plug_name) {      
      if (pluginList[i].plug_handle) {
	dlclose(pluginList[i].plug_handle);
      }
      free(pluginList[i].plug_name);
      i++;
    }
    free(pluginList);
    pluginList = NULL;
  }
  
}
static int locatePluginIndex(const char *pluginname, int alloc)
{
  int idx=0;
  while (pluginList && pluginList[idx].plug_name) {
    if (strcmp(pluginname,pluginList[idx].plug_name)==0) {
      return idx;
    }
    idx++;
  }
  if (alloc) {
    pluginList = realloc(pluginList,sizeof(struct _PluginList)*(idx+2));
    pluginList[idx].plug_name=strdup(pluginname);
    pluginList[idx+1].plug_name=NULL;
    return idx;
  } else {
    return -1;
  }
}

static int dynaloadPlugin(const char *pluginname, int idx)
{
  char pluginbuf[1000];
  pluginbuf[sizeof(pluginbuf)-1]=0;
  strncpy(pluginbuf,CIM_PLUGINDIR "/",sizeof(pluginbuf)-1);
  strncat(pluginbuf,pluginname,sizeof(pluginbuf)-strlen(pluginbuf)-1);
  pluginList[idx].plug_handle = dlopen(pluginbuf,RTLD_LAZY);
  if (pluginList[idx].plug_handle) {
    pluginList[idx].plug_cop4id = 
      dlsym(pluginList[idx].plug_handle,COP4VALID_S);
    pluginList[idx].plug_id4cop = 
      dlsym(pluginList[idx].plug_handle,VALID4COP_S);
    pluginList[idx].plug_getres = 
      dlsym(pluginList[idx].plug_handle,GETRES_S);
    pluginList[idx].plug_freeres = 
      dlsym(pluginList[idx].plug_handle,FREERES_S);
    if (pluginList[idx].plug_cop4id && pluginList[idx].plug_id4cop &&
	pluginList[idx].plug_getres && pluginList[idx].plug_freeres) {
      pluginList[idx].plug_name=strdup(pluginname);
      return 0;
    }
    /* failed to load syms*/
      dlclose(pluginList[idx].plug_handle);
  }
  _OSBASE_TRACE(1,("Failed loading CIM plugin %s at %d: %s\n",pluginbuf,
		   idx, dlerror()));
  pluginList[idx].plug_name=NULL;
  pluginList[idx].plug_handle=NULL;
  pluginList[idx].plug_cop4id=NULL;
  pluginList[idx].plug_id4cop=NULL;
  pluginList[idx].plug_getres=NULL;
  pluginList[idx].plug_freeres=NULL;
  return -1;
}

static CimObjectPathFromValueId * _GetCop4Id(const char *pluginname)
{
  int idx;
  MReadLock(&PdefLock);
  idx=locatePluginIndex(pluginname,0);
  if (idx>=0) {
    MReadUnlock(&PdefLock);
  } else {
    MReadUnlock(&PdefLock);
    MWriteLock(&PdefLock); /* not ideal - but safe */
    idx=locatePluginIndex(pluginname,1);
    /* load the stuff -- in case of failure all fields are set to NULL */
    dynaloadPlugin(pluginname,idx);
    MWriteUnlock(&PdefLock);
  }
  return pluginList[idx].plug_cop4id;
}

static ValueIdFromCimObjectPath * _GetId4Cop(const char *pluginname)
{
  int idx;
  MReadLock(&PdefLock);
  idx=locatePluginIndex(pluginname,0);
  if (idx>=0) {
    MReadUnlock(&PdefLock);
  } else {
    MReadUnlock(&PdefLock);
    MWriteLock(&PdefLock); /* not ideal - but safe */
    idx=locatePluginIndex(pluginname,1);
    /* load the stuff -- in case of failure all fields are set to NULL */
    dynaloadPlugin(pluginname,idx);
    MWriteUnlock(&PdefLock);
  }
  return pluginList[idx].plug_id4cop;
}

static GetResourceClasses * _GetGetRes(const char *pluginname)
{
  int idx;
  MReadLock(&PdefLock);
  idx=locatePluginIndex(pluginname,0);
  if (idx>=0) {
    MReadUnlock(&PdefLock);
  } else {
    MReadUnlock(&PdefLock);
    MWriteLock(&PdefLock); /* not ideal - but safe */
    idx=locatePluginIndex(pluginname,1);
    /* load the stuff -- in case of failure all fields are set to NULL */
    dynaloadPlugin(pluginname,idx);
    MWriteUnlock(&PdefLock);
  }
  return pluginList[idx].plug_getres;
}

static FreeResourceClasses * _GetFreeRes(const char *pluginname)
{
  int idx;
  MReadLock(&PdefLock);
  idx=locatePluginIndex(pluginname,0);
  if (idx>=0) {
    MReadUnlock(&PdefLock);
  } else {
    MReadUnlock(&PdefLock);
    MWriteLock(&PdefLock); /* not ideal - but safe */
    idx=locatePluginIndex(pluginname,1);
    /* load the stuff -- in case of failure all fields are set to NULL */
    dynaloadPlugin(pluginname,idx);
    MWriteUnlock(&PdefLock);
  }
  return pluginList[idx].plug_freeres;
}
