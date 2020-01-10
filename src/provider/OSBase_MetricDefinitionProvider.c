/*
 * $Id: OSBase_MetricDefinitionProvider.c,v 1.9 2009/05/20 19:39:56 tyreld Exp $
 *
 * © Copyright IBM Corp. 2003, 2007, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors:
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: Generic Provider for Metric Defintions served by
 *              the SBLIM Gatherer.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <rrepos.h>
#include <metric.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "OSBase_MetricUtil.h"

#ifdef DEBUG
#define _debug 1
#else
#define _debug 0
#endif

const CMPIBroker * _broker;

/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

static char * _ClassName = "CIM_BaseMetricDefinition";
static char * _FILENAME = "OSBase_MetricDefinitionProvider.c";

#ifdef CMPI_VER_100
#define OSBase_MetricDefinitionProviderSetInstance OSBase_MetricDefinitionProviderModifyInstance 
#endif

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_MetricDefinitionProviderCleanup( CMPIInstanceMI * mi, 
						   const CMPIContext * ctx,
						   CMPIBoolean terminate) { 
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI Cleanup()\n", _FILENAME, _ClassName );
  releaseMetricDefClasses();
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricDefinitionProviderEnumInstanceNames( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref) { 
  CMPIStatus       rc = {CMPI_RC_OK, NULL};
  CMPIObjectPath  *co;
  char           **metricnames;
  int             *metricids;
  int              metricnums;
  int              i;
  
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstanceNames()\n", _FILENAME, _ClassName );

  if (checkRepositoryConnection()) {
    metricnums=getMetricDefsForClass(_broker,ctx,ref,&metricnames,&metricids);
    for (i=0; i<metricnums;i++) {
      co = makeMetricDefPath(_broker,ctx,metricnames[i],metricids[i],
			     CMGetCharPtr(CMGetNameSpace(ref,NULL)),&rc);
      if (co) {
	CMReturnObjectPath(rslt,co);
      } else {
	CMSetStatusWithChars( _broker, &rc, 
			      CMPI_RC_ERR_FAILED, 
			      "Could not construct instance name." ); 
      }
    }
    releaseMetricDefs(metricnames,metricids);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Repository Service not active" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderEnumInstances( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) { 
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  CMPIInstance  *ci;
  char         **metricnames;
  int           *metricids;
  int            metricnums;
  int           i;

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstances()\n", _FILENAME, _ClassName );

  if (checkRepositoryConnection()) {
    metricnums=getMetricDefsForClass(_broker,ctx,ref,&metricnames,&metricids);
    for (i=0; i<metricnums;i++) {
      ci = makeMetricDefInst(_broker,ctx,metricnames[i],metricids[i],
			     CMGetCharPtr(CMGetNameSpace(ref,NULL)),&rc);
      if (ci) {
	CMReturnInstance(rslt,ci);
      } else {
	CMSetStatusWithChars( _broker, &rc, 
			      CMPI_RC_ERR_FAILED, 
			      "Could not construct instance." ); 
      }
    } 
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Repository Service not active" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderGetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop, 
           const char ** properties) {
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  char             mname[300];
  int              mid;
  CMPIData         idData;
  
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI GetInstance()\n", _FILENAME, _ClassName );
  
  idData = CMGetKey(cop,"Id",NULL);
  if (idData.value.string && checkRepositoryConnection()) {
    if (parseMetricDefId(CMGetCharPtr(idData.value.string),mname,&mid) == 0) {
      ci=makeMetricDefInst(_broker,ctx,mname,mid,
			   CMGetCharPtr(CMGetNameSpace(cop,NULL)),&rc);
      if( ci == NULL ) {
	if( _debug ) {
	  if( rc.msg != NULL )
	    { fprintf(stderr,"rc.msg: %s\n",CMGetCharPtr(rc.msg)); }
	}
	if (rc.rc == CMPI_RC_OK) {
	  /* it can't be OK if we didn't get an instance */
	  CMSetStatusWithChars( _broker, &rc, 
				CMPI_RC_ERR_NOT_FOUND, 
				"Invalid metric definition id" ); 
	}
      } else {
	CMReturnInstance( rslt, ci );
      }
    } else {
      CMSetStatusWithChars( _broker, &rc, 
			    CMPI_RC_ERR_INVALID_PARAMETER, 
			    "Invalid Object Path Key \"Id\"" ); 
    }
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Repository Service not active" ); 
  }
  CMReturnDone(rslt);
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderCreateInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop, 
           const CMPIInstance * ci) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI CreateInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderSetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop,
           const CMPIInstance * ci, 
           const char **properties) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI SetInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderDeleteInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * cop) {
  CMPIStatus rc = {CMPI_RC_OK, NULL}; 

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI DeleteInstance()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}

CMPIStatus OSBase_MetricDefinitionProviderExecQuery( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char * lang, 
           const char * query) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI ExecQuery()\n", _FILENAME, _ClassName );

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}



/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMInstanceMIStub( OSBase_MetricDefinitionProvider, 
                  OSBase_MetricDefinitionProvider, 
                  _broker, 
                  CMNoHook);

/* ---------------------------------------------------------------------------*/
/*              end of OSBase_MetricDefinitionProvider                      */
/* ---------------------------------------------------------------------------*/

