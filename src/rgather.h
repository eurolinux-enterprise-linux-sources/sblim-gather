/*
 * $Id: rgather.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Remote Gatherer API
 */

#ifndef RGATHER_H
#define RGATHER_H


#ifdef __cplusplus
extern "C" {
#endif

#include "gather.h"
#include "commheap.h"

int rgather_load();
int rgather_unload();

int rgather_init(); 
int rgather_start(); 
int rgather_stop(); 
int rgather_terminate(); 

int rmetricplugin_add(const char *pluginname);
int rmetricplugin_remove(const char *pluginname);
int rmetricplugin_list(const char *pluginname, PluginDefinition **pdef, 
		       COMMHEAP ch);
  
int rgather_status(GatherStatus *gs); 

#ifdef __cplusplus
}
#endif

#endif
