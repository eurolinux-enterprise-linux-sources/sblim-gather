/*
 * $Id: OSBase_MetricInstanceProvider.c,v 1.9 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: Association Provider for CIM_MetricInstance
 * 
 */


#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>
#include <rrepos.h>
#include <OSBase_Common.h>
#include "OSBase_MetricUtil.h"

#define LOCALCLASSNAME "Linux_MetricInstance"

static const CMPIBroker * _broker;

#ifdef CMPI_VER_100
#define OSBase_MetricInstanceProviderSetInstance OSBase_MetricInstanceProviderModifyInstance 
#endif


/* ------------------------------------------------------------------ *
 * Simplified Utility Functions
 * no support for assocClass, resultClass, etc.
 * ------------------------------------------------------------------ */

static CMPIInstance * _makeRefInstance(const CMPIObjectPath *defp,
				       const CMPIObjectPath *valp)
{
  CMPIObjectPath *co = CMNewObjectPath(_broker,NULL,LOCALCLASSNAME,NULL);
  CMPIInstance   *ci = NULL;
  if (co) {
    CMSetNameSpaceFromObjectPath(co,defp);
    ci = CMNewInstance(_broker,co, NULL);
    if (ci) {
      CMSetProperty(ci,"Antecedent",&defp,CMPI_ref);
      CMSetProperty(ci,"Dependent",&valp,CMPI_ref);
    }
  }
  return ci;
}

static CMPIObjectPath * _makeRefPath(const CMPIObjectPath *defp,
				     const CMPIObjectPath *valp)
{
  CMPIObjectPath *co = CMNewObjectPath(_broker,NULL,LOCALCLASSNAME,NULL);
  if (co) {
    CMSetNameSpaceFromObjectPath(co,defp);
    CMAddKey(co,"Antecedent",&defp,CMPI_ref);
    CMAddKey(co,"Dependent",&valp,CMPI_ref);
  }
  return co;
}

static CMPIStatus associatorHelper( const CMPIResult * rslt,
				    const CMPIContext * ctx,
				    const CMPIObjectPath * cop,
				    int associators, int names ) 
{
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  CMPIString     *clsname, *namesp;
  CMPIObjectPath *co;
  CMPIInstance   *ci;
  CMPIData        iddata;
  char            defclass[500];
  char            metricname[500];
  int             metricid;
  int             i;
  ValueRequest    vr;
  COMMHEAP        ch;

  _OSBASE_TRACE(3,("%s associatorHelper() - associators = %d, names = %d\n",LOCALCLASSNAME,associators, names));
    
  clsname=CMGetClassName(cop,NULL);
  namesp=CMGetNameSpace(cop,NULL);
  /*
   * Check if the object path belongs to a supported class
   */ 
  if (CMClassPathIsA(_broker,cop,"CIM_BaseMetricDefinition",NULL)) {
    _OSBASE_TRACE(4,("%s associatorHelper(): lhs is %s\n",LOCALCLASSNAME,CMGetCharPtr(clsname)));
    iddata = CMGetKey(cop,"Id",NULL);
    /* Metric Definition - we search for Metric Values for this Id */
    if (iddata.value.string &&
	parseMetricDefId(CMGetCharPtr(iddata.value.string),
			 metricname,&metricid) == 0) {
      if (checkRepositoryConnection()) {
	ch=ch_init();
	vr.vsId=metricid;
	vr.vsResource=NULL;
	vr.vsSystemId=NULL;
	vr.vsFrom=vr.vsTo=0;
	vr.vsNumValues=1; /* restrict to one/latest value per resource */
	if (rrepos_get(&vr,ch) == 0 ) {
	  for (i=0; i < vr.vsNumValues; i++) {
	    co = makeMetricValuePath( _broker, ctx,
				      metricname, metricid,
				      &vr.vsValues[i], 
				      cop, &st );
	    if (co == NULL) {
	      continue;
	    }
	    if (names && associators) {
	      CMReturnObjectPath(rslt,co);
	    } else if (!names && associators) {
	      ci = makeMetricValueInst( _broker, ctx, metricname, metricid,
					&vr.vsValues[i],
					vr.vsDataType,
					cop, &st );
	      if (ci) {
		CMReturnInstance(rslt,ci);	      
	      }
	    } else if (names) {
	      CMReturnObjectPath(rslt,_makeRefPath(cop, co));
	    } else {
	      CMReturnInstance(rslt,_makeRefInstance(cop, co));
	    }
	  }
	}
	ch_release(ch);
      }
    }
  } else if (CMClassPathIsA(_broker,cop,"CIM_BaseMetricValue",NULL)) {
    _OSBASE_TRACE(4,("%s associatorHelper(): lhs is %s\n",LOCALCLASSNAME,CMGetCharPtr(clsname)));
    iddata = CMGetKey(cop,"MetricDefinitionId",NULL);
    /* Must be a metric value */
    if (iddata.value.string &&
	parseMetricDefId(CMGetCharPtr(iddata.value.string),
			 metricname,&metricid) == 0) {
      if (metricDefClassName(_broker,ctx,
			     CMGetCharPtr(namesp),
			     defclass,metricname,metricid) == 0) {
	co = makeMetricDefPath(_broker,ctx,
			       metricname,metricid,CMGetCharPtr(namesp),
			       &st); 
	if (co) {
	  if (names && associators) {
	    CMReturnObjectPath(rslt,co);
	  } else if (!names && associators) {
	    ci = makeMetricDefInst( _broker, ctx, metricname, metricid,
				    CMGetCharPtr(namesp), &st );
	    if (ci) {
	      CMReturnInstance(rslt,ci);	      
	    }
	  } else if (names) {
	    CMReturnObjectPath(rslt,_makeRefPath(co, cop));
	  } else {
	    CMReturnInstance(rslt,_makeRefInstance(co, cop));
	  }
	}
      }
      
    }
  } else {
    _OSBASE_TRACE(4,("%s associatorHelper(): unsupported class %s\n",LOCALCLASSNAME,CMGetCharPtr(clsname)));
  }
  CMReturnDone(rslt);
  
  return st;
}

/* ------------------------------------------------------------------ *
 * Instance MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus OSBase_MetricInstanceProviderCleanup( CMPIInstanceMI * mi, 
						 const CMPIContext * ctx,
						 CMPIBoolean terminate) 
{
  releaseMetricDefClasses();
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus OSBase_MetricInstanceProviderEnumInstanceNames( CMPIInstanceMI * mi, 
					   const CMPIContext * ctx, 
					   const CMPIResult * rslt, 
					   const CMPIObjectPath * ref) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus OSBase_MetricInstanceProviderEnumInstances( CMPIInstanceMI * mi, 
				       const CMPIContext * ctx, 
				       const CMPIResult * rslt, 
				       const CMPIObjectPath * ref, 
				       const char ** properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}


CMPIStatus OSBase_MetricInstanceProviderGetInstance( CMPIInstanceMI * mi, 
				     const CMPIContext * ctx, 
				     const CMPIResult * rslt, 
				     const CMPIObjectPath * cop, 
				     const char ** properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus OSBase_MetricInstanceProviderCreateInstance( CMPIInstanceMI * mi, 
					const CMPIContext * ctx, 
					const CMPIResult * rslt, 
					const CMPIObjectPath * cop, 
					const CMPIInstance * ci) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus OSBase_MetricInstanceProviderSetInstance( CMPIInstanceMI * mi, 
				     const CMPIContext * ctx, 
				     const CMPIResult * rslt, 
				     const CMPIObjectPath * cop,
				     const CMPIInstance * ci, 
				     const char **properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus OSBase_MetricInstanceProviderDeleteInstance( CMPIInstanceMI * mi, 
					const CMPIContext * ctx, 
					const CMPIResult * rslt, 
					const CMPIObjectPath * cop) 
{ 
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus OSBase_MetricInstanceProviderExecQuery( CMPIInstanceMI * mi, 
				   const CMPIContext * ctx, 
				   const CMPIResult * rslt, 
				   const CMPIObjectPath * cop, 
				   const char * lang, 
				   const char * query) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

/* ------------------------------------------------------------------ *
 * Association MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus OSBase_MetricInstanceProviderAssociationCleanup( CMPIAssociationMI * mi,
							    const CMPIContext * ctx,
							    CMPIBoolean terminate) 
{
  releaseMetricDefClasses();
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Association MI Functions
 * ------------------------------------------------------------------ */

CMPIStatus OSBase_MetricInstanceProviderAssociators( CMPIAssociationMI * mi,
				       const CMPIContext * ctx,
				       const CMPIResult * rslt,
				       const CMPIObjectPath * cop,
				       const char * assocClass,
				       const char * resultClass,
				       const char * role,
				       const char * resultRole,
				       const char ** propertyList ) 
{
  return associatorHelper(rslt,ctx,cop,1,0);
}

CMPIStatus OSBase_MetricInstanceProviderAssociatorNames( CMPIAssociationMI * mi,
					   const CMPIContext * ctx,
					   const CMPIResult * rslt,
					   const CMPIObjectPath * cop,
					   const char * assocClass,
					   const char * resultClass,
					   const char * role,
					   const char * resultRole ) 
{
  return associatorHelper(rslt,ctx,cop,1,1);
}

CMPIStatus OSBase_MetricInstanceProviderReferences( CMPIAssociationMI * mi,
				      const CMPIContext * ctx,
				      const CMPIResult * rslt,
				      const CMPIObjectPath * cop,
				      const char * assocClass,
				      const char * role,
				      const char ** propertyList ) 
{
  return associatorHelper(rslt,ctx,cop,0,0);
}


CMPIStatus OSBase_MetricInstanceProviderReferenceNames( CMPIAssociationMI * mi,
					  const CMPIContext * ctx,
					  const CMPIResult * rslt,
					  const CMPIObjectPath * cop,
					  const char * assocClass,
					  const char * role) 
{
  return associatorHelper(rslt,ctx,cop,0,1);
}

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * 
 * NOTE: This is an example using the convenience macros. This is OK 
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub( OSBase_MetricInstanceProvider,
		  OSBase_MetricInstanceProvider,
		  _broker,
		  CMNoHook);

/* ------------------------------------------------------------------ *
 * Association MI Factory
 * ------------------------------------------------------------------ */

CMAssociationMIStub( OSBase_MetricInstanceProvider,
		     OSBase_MetricInstanceProvider,
		     _broker,
		     CMNoHook);
