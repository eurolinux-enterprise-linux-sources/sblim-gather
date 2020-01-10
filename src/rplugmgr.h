/*
 * $Id: rplugmgr.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: 
 *
 * Description: Repository Plugin Manager Interface
 */

#ifndef RPLUGMGR_H
#define RPLUGMGR_H

#include "metric.h"
#include "rplugin.h"

/* plugin structure */
typedef struct _RepositoryPlugin {
  /* supplied by caller */
  char                        *rpName;
  MetricRegisterId            *rpRegister;
  /* supplied by plugin */
  void                        *rpHandle;
  size_t                       rpNumMetricCalcDefs;
  MetricCalculationDefinition *rpMetricCalcDefs;
} RepositoryPlugin;

int RP_SetPluginPath(const char *pluginpath);
char* RP_GetPluginPath();
int RP_Load(RepositoryPlugin *plugin);
int RP_Unload(RepositoryPlugin *plugin);

#endif
