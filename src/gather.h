/*
 * $Id: gather.h,v 1.7 2009/05/20 19:39:55 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2004, 2009
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
 * Description: Local Gatherer Interface
 */

#ifndef GATHER_H
#define GATHER_H

#include <stdlib.h>
#include "commheap.h"

#ifdef __cplusplus
extern "C" {
#endif

int gather_init(); 
int gather_start(); 
int gather_stop(); 
int gather_terminate(); 

int metricplugin_loadall();
int metricplugin_add(const char *pluginname);
int metricplugin_remove(const char *pluginname);

typedef struct _PluginDefinition {
  int      pdId;
  char    *pdName; 
} PluginDefinition;

int metricplugin_list(const char *pluginname,
		      PluginDefinition **pdef, 
		      COMMHEAP ch);


int metricpluginname_list(char ***pluginname,
			  COMMHEAP ch);

typedef struct _GatherStatus {
  short    gsInitialized;
  short    gsSampling;
  unsigned gsNumPlugins;
  unsigned gsNumMetrics;
} GatherStatus;

void gather_status(GatherStatus *gs); 

#ifdef __cplusplus
}
#endif

#endif
