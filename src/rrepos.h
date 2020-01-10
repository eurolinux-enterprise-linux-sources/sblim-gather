/*
 * $Id: rrepos.h,v 1.9 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: 
 *
 * Description: Remote Repository API
 */

#ifndef RREPOS_H
#define RREPOS_H


#ifdef __cplusplus
extern "C" {
#endif

#include "commheap.h"
#include "metric.h"
#include "repos.h"
#include "gather.h"

/*
 * Session check, meaning of return codes
 * 0  Session OK
 * 1  Session newly established
 * -1 Session cannot be established
 */
int rrepos_sessioncheck();

/*
 * Register Plugin Definition, meaning of return codes
 * 0  Successfully registered
 * -1 Could not register
 */
int rrepos_register(const PluginDefinition *pdef);

/*
 * Store value in remote repository
 * 0  OK
 * -1 Failure
 */
int rrepos_put(const char *reposplugin, const char *metric,MetricValue *mv);

/*
 * Retrieve top num value from remote repository
 * 0  OK
 * -1 Failure
 */
int rrepos_getfiltered(ValueRequest *vs, COMMHEAP ch, size_t num, int ascending);

/*
 * Retrieve value from remote repository
 * 0  OK
 * -1 Failure
 */
int rrepos_get(ValueRequest *vs, COMMHEAP ch);

/*
 * Set global filter condition for get requests
 * 0  OK
 * -1 Failure
 */
int rrepos_setglobalfilter(size_t num, int asc);

/*
 * Set global filter condition for get requests
 * 0  OK
 * -1 Failure
 */
int rrepos_getglobalfilter(size_t *num, int *asc);

/*
 * Control interface, spawn repository daemon
 */
int rrepos_load();

/*
 * Control interface, kill repository daemon
 */
int rrepos_unload();

/*
 * Control interface, initialize repository daemon
 */
int rrepos_init(); 

/*
 * Control interface, terminate repository daemon
 */
int rrepos_terminate(); 

/*
 * Control interface, return status of the repository daemon
 */
int rrepos_status(RepositoryStatus *rs); 

/*
 * Control interface, load a repository plugin
 */
int rreposplugin_add(const char *pluginname);

/*
 * Control interface, remove a repository plugin
 */
int rreposplugin_remove(const char *pluginname);

/*
 * Control interface, list repository plugin definitions
 */
int rreposplugin_list(const char *pluginname,
		      RepositoryPluginDefinition **pdef, 
		      COMMHEAP ch);

/*
 * Control interface, subscribe and unsubscribe to repository events
 */
int rrepos_subscribe(SubscriptionRequest *sr,  SubscriptionCallback *scb);


int rrepos_unsubscribe(SubscriptionRequest *sr,  SubscriptionCallback *scb);

/*
 * Retrieve resource list from remote repository
 * 0  OK
 * -1 Failure
 */
int rreposresource_list(const char * metricid,
			MetricResourceId **rid, 
			COMMHEAP ch);

#ifdef __cplusplus
}
#endif

#endif
