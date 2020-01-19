/*
 * $Id: OSBase_MetricUtil.h,v 1.12 2012/05/17 01:02:42 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Metric Provider Utility Functions
 */

#ifndef OSBASE_METRICUTIL_H
#define OSBASE_METRICUTIL_H

#include <time.h>
#include <cmpidt.h>
#include <repos.h>

int checkRepositoryConnection();

/* maintain cache of CIM class to id mappings */
int refreshMetricDefClasses(const CMPIBroker *broker, const CMPIContext *ctx, 
			    const char *namesp, int force);
void releaseMetricDefClasses();

/* retrieve CIM class name for given ids */
int metricDefClassName(const CMPIBroker *broker, const CMPIContext *ctx, 
		       const char *namesp, char *clsname, 
		       const char *name, int id);
int metricValueClassName(const CMPIBroker *broker, const CMPIContext *ctx, 
			 const char *namesp,char *clsname, 
			 const char *name, int id);
int metricPluginName(const CMPIBroker *broker, const CMPIContext *ctx, 
		     const char *namesp,char *pluginname, 
		     const char *name, int id);

/* metric id to CIM key mappings */
char * makeMetricDefIdFromCache(const CMPIBroker *broker, const CMPIContext *ctx,
				const char * namesp, char * defid, int id);
char * makeMetricValueIdFromCache(const CMPIBroker *broker, const CMPIContext *ctx,
				  const char * namesp, char * valid, int id,
				  const char * resource, const char * systemid,
				  time_t timestamp);
char * makeMetricDefId(char * defid, const char * name, int id);
int parseMetricDefId(const char * defid,
		     char * name, int * id);
char * makeMetricValueId(char * instid, const char * name, int id, 
			 const char * resource, const char * systemid,
			 time_t timestamp);
int parseMetricValueId(const char * instid,
		       char * name, int * id, char * resource,
		       char * systemid, time_t * timestamp);

/* plugin name for metric definition class name */
int getPluginNamesForValueClass(const CMPIBroker *broker, const CMPIContext *ctx, 
				const CMPIObjectPath *cop, char ***pluginnames);
void releasePluginNames(char **pluginnames);
int getMetricDefsForClass(const CMPIBroker *broker, const CMPIContext *ctx, 
			  const CMPIObjectPath* cop,
			  char ***mnames, int **mids);
void releaseMetricDefs(char **mnames,int *mids); 

/* resource class to metric id mapping */
int getMetricIdsForResourceClass(const CMPIBroker *broker, const CMPIContext *ctx, 
				 const CMPIObjectPath* cop,
				 char ***metricnames,
				 int **mids, 
				 char ***resourceids,
				 char ***systemids);
void releaseMetricIds(char **metricnames, int *mids, char **resourceids,
		      char **systemids); 

/* support for instance and object path construction */
CMPIString * val2string(const CMPIBroker * broker, const ValueItem *val,
			unsigned   datatype);
CMPIInstance * makeMetricValueInst(const CMPIBroker * broker, 
				   const CMPIContext * ctx,
				   const char * defname,
				   int defid,
				   const ValueItem *val,
				   unsigned   datatype,
				   const CMPIObjectPath *cop,
                                   const char ** props,
				   CMPIStatus * rc);
CMPIObjectPath * makeMetricValuePath(const CMPIBroker * broker,
				     const CMPIContext * ctx,
				     const char * defname,
				     int defid,
				     const ValueItem *val, 
				     const CMPIObjectPath *cop,
				     CMPIStatus * rc);

CMPIObjectPath * makeMetricDefPath(const CMPIBroker * broker,
				   const CMPIContext * ctx,
				   const char * defname,
				   int defid,
				   const char * namesp,
				   CMPIStatus * rc);
CMPIInstance * makeMetricDefInst(const CMPIBroker * broker,
				 const CMPIContext * ctx,
				 const char * defname,
				 int defid,
				 const char * namesp,
                                 const char ** props,
				 CMPIStatus * rc);

CMPIObjectPath * makeResourcePath(const CMPIBroker * broker,
				  const CMPIContext * ctx,
				  const char * namesp,
				  const char * defname,
				  int defid,
				  const char * resourcename,
				  const char * systemid);

void computeResourceNamespace(const CMPIObjectPath *rescop,
			      const CMPIObjectPath *mcop,
			      const char *systemid);

#endif
