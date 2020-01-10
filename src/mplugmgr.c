/*
 * $Id: mplugmgr.c,v 1.4 2009/05/20 19:39:55 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2009
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
 * Description: Metric Plugin Manager
 * Does dynamic loading of metric plugins.
 */

#include "mplugmgr.h"
#include "mplugin.h"
#include <mlog.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

static char * metricPluginPath = NULL;

char* MP_GetPluginPath()
{
  return metricPluginPath;
}

int MP_SetPluginPath(const char * pluginpath)
{
  if (metricPluginPath) {
    free(metricPluginPath);
  }
  metricPluginPath = strdup(pluginpath);
  return 0;
}

int MP_Load (MetricPlugin *plugin)
{
  MetricsDefined  *mdef;
  MetricStartStop *mss;
  char * pluginName = NULL;

  if (plugin==NULL || plugin->mpName==NULL || plugin->mpName[0]==0) {
    m_log(M_ERROR,M_SHOW,"Null plugin name specified in load request\n");
    return -1;
  }
  pluginName = malloc( (metricPluginPath ? strlen(metricPluginPath) : 0) +
		       strlen(plugin->mpName) + 2);
  if (metricPluginPath) {
    strcpy(pluginName,metricPluginPath);
    strcat(pluginName,"/");
  } else {
    strcpy(pluginName,"");
  }
  strcat(pluginName,plugin->mpName);
  plugin->mpHandle = dlopen(pluginName,RTLD_LAZY);
  if (!plugin->mpHandle) {
    m_log(M_ERROR,M_SHOW,"Error loading plugin library %s: %s\n", pluginName, 
	  dlerror());
    free(pluginName);
    return -1;
  }
  free(pluginName);
  mdef = (MetricsDefined*)dlsym(plugin->mpHandle,METRIC_DEFINITIONPROC_S);
  if (!mdef) {
    m_log(M_ERROR,M_SHOW,"Error locating " METRIC_DEFINITIONPROC_S " in %s: %s\n", 
	    plugin->mpName,dlerror());
    dlclose(plugin->mpHandle);
    return -1;
  }
  mss = (MetricStartStop*)dlsym(plugin->mpHandle,METRIC_STARTSTOPPROC_S);
  if(!mss) {
    m_log(M_ERROR,M_SHOW,"Error locating " METRIC_STARTSTOPPROC_S " in %s: %s\n", 
	    plugin->mpName, dlerror());
    dlclose(plugin->mpHandle);
    return -1;
  }
  /* clean out some fields, then get metrics */
  plugin->mpNumMetricDefs=0;
  plugin->mpMetricDefs=NULL;
  if (mdef(plugin->mpRegister,
	   plugin->mpName,
	   &plugin->mpNumMetricDefs,
	   &plugin->mpMetricDefs)) {
    m_log(M_ERROR,M_SHOW,"Couldn't get metrics for plugin %s\n",plugin->mpName); 
    dlclose(plugin->mpHandle);
    return -1;
  }
  /* Start plugin */
  if (mss(1)) {
    m_log(M_ERROR,M_SHOW,"Couldn't start plugin %s\n",plugin->mpName); 
    dlclose(plugin->mpHandle);
    return -1;
  }
  return 0;
}

int MP_Unload(MetricPlugin *plugin)
{
  MetricStartStop *mss;
  
  if (plugin==NULL || plugin->mpName==NULL || plugin->mpHandle==NULL) {
    m_log(M_ERROR,M_SHOW,"Null plugin specified\n");
    return -1;
  }
  mss = (MetricStartStop*)dlsym(plugin->mpHandle,METRIC_STARTSTOPPROC_S);
  if(!mss) {
    m_log(M_ERROR,M_SHOW,"Error locating " METRIC_STARTSTOPPROC_S " in %s: %s\n", 
	    plugin->mpName,dlerror());
    return -1;
  }
  /* Stop plugin */
  if (mss(0)) {
    m_log(M_ERROR,M_SHOW,"Couldn't stop plugin - forcing unload\n"); 
  }
  if (dlclose(plugin->mpHandle)) {
    m_log(M_ERROR,M_SHOW,"Couldn't unload plugin\n");
    return -1;
  }
  plugin->mpHandle=NULL; /* disallow accidental reuse */
  return 0;
}
