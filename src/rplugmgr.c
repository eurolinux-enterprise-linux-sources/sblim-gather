/*
 * $Id: rplugmgr.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Repository Plugin Manager
 * Does dynamic loading of repository plugins.
 */

#include "rplugmgr.h"
#include "rplugin.h"
#include <mlog.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

static char * repositoryPluginPath = NULL;

char* RP_GetPluginPath()
{
  return repositoryPluginPath;
}

int RP_SetPluginPath(const char * pluginpath)
{
  if (repositoryPluginPath) {
    free(repositoryPluginPath);
  }
  repositoryPluginPath = strdup(pluginpath);
  return 0;
}

int RP_Load (RepositoryPlugin *plugin)
{
  RepositoryMetricsDefined  *mdef;
  char * pluginName = NULL;

  if (plugin==NULL || plugin->rpName==NULL || plugin->rpName[0]==0) {
    m_log(M_ERROR,M_SHOW,"Null plugin name specified in load request\n");
    return -1;
  }
  pluginName = malloc( (repositoryPluginPath ? strlen(repositoryPluginPath) : 0) +
		       strlen(plugin->rpName) + 2);
  if (repositoryPluginPath) {
    strcpy(pluginName,repositoryPluginPath);
    strcat(pluginName,"/");
  } else {
    strcpy(pluginName,"");
  }
  strcat(pluginName,plugin->rpName);
  plugin->rpHandle = dlopen(pluginName,RTLD_LAZY);
  if (!plugin->rpHandle) {
    m_log(M_ERROR,M_SHOW,"Error loading plugin library %s: %s\n", pluginName, 
	    dlerror());
    free(pluginName);
    return -1;
  }
  free(pluginName);
  mdef = (RepositoryMetricsDefined*)dlsym(plugin->rpHandle,REPOSITORYMETRIC_DEFINITIONPROC_S);
  if (!mdef) {
    m_log(M_ERROR,M_SHOW,
	    "Error locating " REPOSITORYMETRIC_DEFINITIONPROC_S " in %s: %s\n", 
	    plugin->rpName,dlerror());
    dlclose(plugin->rpHandle);
    return -1;
  }
  /* clean out some fields, then get metrics */
  plugin->rpNumMetricCalcDefs=0;
  plugin->rpMetricCalcDefs=NULL;
  if (mdef(plugin->rpRegister,
	   plugin->rpName,
	   &plugin->rpNumMetricCalcDefs,
	   &plugin->rpMetricCalcDefs)) {
    m_log(M_ERROR,M_SHOW,"Couldn't get metrics for repository plugin %s\n",
	    plugin->rpName); 
    dlclose(plugin->rpHandle);
    return -1;
  }
  return 0;
}

int RP_Unload(RepositoryPlugin *plugin)
{  
  if (plugin==NULL || plugin->rpName==NULL || plugin->rpHandle==NULL) {
    m_log(M_ERROR,M_SHOW,"Null plugin specified\n");
    return -1;
  }
  if (dlclose(plugin->rpHandle)) {
    m_log(M_ERROR,M_SHOW,"Couldn't unload plugin\n");
    return -1;
  }
  plugin->rpHandle=NULL; /* disallow accidental reuse */
  return 0;
}
