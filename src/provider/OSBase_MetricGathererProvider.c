/*
 * $Id: OSBase_MetricGathererProvider.c,v 1.13 2012/10/25 23:13:52 tyreld Exp $
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
 * Description: Provider for the SBLIM Gatherer.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <metric.h>
#include <rgather.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <OSBase_Common.h> 
#include <cmpiOSBase_Common.h> 

#ifdef DEBUG
#define _debug 1
#else
#define _debug 0
#endif

const CMPIBroker * _broker;

/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

static char * _ClassName = "Linux_MetricGatherer";
static char * _Name = "gatherd";
static char * _FILENAME = "OSBase_MetricGathererProvider.c";

static char  _false=0;

#ifdef CMPI_VER_100
#define OSBase_MetricGathererProviderSetInstance OSBase_MetricGathererProviderModifyInstance 
#endif

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_MetricGathererProviderCleanup( CMPIInstanceMI * mi, 
						 const CMPIContext * ctx, 
						 CMPIBoolean terminate) { 
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI Cleanup()\n", _FILENAME, _ClassName );
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricGathererProviderEnumInstanceNames( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref) { 
  CMPIObjectPath * op = NULL;
  CMPIStatus       rc = {CMPI_RC_OK, NULL};
  
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstanceNames()\n", _FILENAME, _ClassName );

  /* we assume there is exactly one of us */
  op=CMNewObjectPath(_broker,CMGetCharPtr(CMGetNameSpace(ref,NULL)),_ClassName,
		     NULL);
  if (op) {
    CMAddKey(op,"CreationClassName",_ClassName,CMPI_chars);
    CMAddKey(op,"Name",_Name,CMPI_chars);
    CMAddKey(op,"SystemCreationClassName",CSCreationClassName,CMPI_chars);
    CMAddKey(op,"SystemName",get_system_name(),CMPI_chars);
    CMReturnObjectPath(rslt,op);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not build object path" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricGathererProviderEnumInstances( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) { 
  CMPIObjectPath * op = NULL;
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  GatherStatus   gs;
  short          ival;
  short          nump;
  short          numm;
  char           boolval;
  
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI EnumInstances()\n", _FILENAME, _ClassName );

  op=CMNewObjectPath(_broker,CMGetCharPtr(CMGetNameSpace(ref,NULL)),_ClassName,
		     NULL);
  if (op) {
    
    ci=CMNewInstance(_broker,op,NULL);
  } 
  if (ci) {
    CMSetPropertyFilter(ci, properties, NULL);

    CMSetProperty(ci,"CreationClassName",_ClassName,CMPI_chars);
    CMSetProperty(ci,"Name",_Name,CMPI_chars);
    CMSetProperty(ci,"SystemCreationClassName",CSCreationClassName,CMPI_chars);
    CMSetProperty(ci,"SystemName",get_system_name(),CMPI_chars);

    CMSetProperty(ci,"Release",PACKAGE_VERSION,CMPI_chars);

    if (rgather_status(&gs)) {
      CMSetProperty(ci,"Started",&_false,CMPI_boolean);
      ival=0; /* unknown */
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
    } else {
      boolval=gs.gsInitialized;
      CMSetProperty(ci,"Started",&boolval,CMPI_boolean);
      if (gs.gsInitialized)
	ival=2;
      else
	ival=0;
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
      boolval=gs.gsSampling;
      CMSetProperty(ci,"Sampling",&boolval,CMPI_boolean);
      nump = gs.gsNumPlugins; 
      CMSetProperty(ci,"NumberOfPlugins",&nump,CMPI_uint16);
      numm = gs.gsNumMetrics;
      CMSetProperty(ci,"NumberOfMetrics",&numm,CMPI_uint16);
    }
    CMReturnInstance(rslt,ci);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not build object path" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricGathererProviderGetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) {
  CMPIObjectPath * op = NULL;
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  GatherStatus   gs;
  short          ival;
  short          nump;
  short          numm;
  char           boolval;

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI GetInstance()\n", _FILENAME, _ClassName );
  op=CMNewObjectPath(_broker,CMGetCharPtr(CMGetNameSpace(ref,NULL)),_ClassName,
		     NULL);
  if (op) {

    ci=CMNewInstance(_broker,op,NULL);
  } 
  if (ci) {
    CMSetPropertyFilter(ci, properties, NULL);

    CMSetProperty(ci,"CreationClassName",_ClassName,CMPI_chars);
    CMSetProperty(ci,"Name",_Name,CMPI_chars);
    CMSetProperty(ci,"SystemCreationClassName",CSCreationClassName,CMPI_chars);
    CMSetProperty(ci,"SystemName",get_system_name(),CMPI_chars);

    CMSetProperty(ci,"Release",PACKAGE_VERSION,CMPI_chars);

    if (rgather_status(&gs)) {
      CMSetProperty(ci,"Started",&_false,CMPI_boolean);
      ival=0; /* unknown */
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
    } else {
      boolval=gs.gsInitialized;
      CMSetProperty(ci,"Started",&boolval,CMPI_boolean);
      if (gs.gsInitialized)
	ival=2;
      else
	ival=0;
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
      boolval=gs.gsSampling;
      CMSetProperty(ci,"Sampling",&boolval,CMPI_boolean);
      nump = gs.gsNumPlugins;
      CMSetProperty(ci,"NumberOfPlugins",&nump,CMPI_uint16);
      numm = gs.gsNumMetrics;
      CMSetProperty(ci,"NumberOfMetrics",&numm,CMPI_uint16);
    }
    CMReturnInstance(rslt,ci);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Gatherer Service not active" ); 
  }
  CMReturnDone(rslt);
  return rc;
}

CMPIStatus OSBase_MetricGathererProviderCreateInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricGathererProviderSetInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricGathererProviderDeleteInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricGathererProviderExecQuery( CMPIInstanceMI * mi, 
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
/*                      Method Provider Interface                             */
/* ---------------------------------------------------------------------------*/
CMPIStatus OSBase_MetricGathererProviderMethodCleanup( CMPIMethodMI * mi, 
						       const CMPIContext * ctx, 
						       CMPIBoolean terminate) 
{
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI MethodCleanup()\n", _FILENAME, _ClassName );

  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricGathererProviderInvokeMethod( CMPIMethodMI * mi, 
						      const CMPIContext * ctx, 
						      const CMPIResult * rslt,
						      const CMPIObjectPath * cop,
						      const char * method,
						      const CMPIArgs * in,
						      CMPIArgs * out)
{
  CMPIStatus   st = {CMPI_RC_OK,NULL};
  GatherStatus gs;
  CMPIData     data;
  CMPIEnumeration * en;
  CMPIObjectPath  * copPlugin;
  int          stat;
  char         boolval;  

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI InvokeMethod()\n", _FILENAME, _ClassName );

  if (rgather_status(&gs)) {
    gs.gsInitialized=0;
    gs.gsSampling=0;
  }

  if (strcasecmp(method,"startservice")==0) {
    if( _debug )
      fprintf( stderr, "--- invoking startservice()\n");
    if (!gs.gsInitialized) {
      rgather_load(); /* load daemon, if necessary */
      sleep(1);
      if (rgather_init()==0) {
	stat=0;
	/* now load plugins */
	copPlugin = CMNewObjectPath(_broker,
				    CMGetCharPtr(CMGetNameSpace(cop,NULL)),
				    "Linux_MetricPlugin",NULL);
	if (copPlugin) {
	  en = CBEnumInstances(_broker,ctx,copPlugin,NULL,NULL);
	  while (CMHasNext(en,NULL)) {
	    data = CMGetNext(en,NULL);
	    if (data.value.inst) {
	      data = CMGetProperty(data.value.inst,"MetricPluginName",NULL);
	      if (data.type == CMPI_string && data.value.string) {
		rmetricplugin_add(CMGetCharPtr(data.value.string));
	      }
	    }
	  }
	}
      } else
	stat=1;
    } else {
      stat=0;
    }
    CMReturnData(rslt,&stat,CMPI_uint32);
  } else if (strcasecmp(method,"stopservice") ==0) {
    if( _debug )
      fprintf( stderr, "--- invoking stopservice()\n");
    if (gs.gsInitialized) {
      if (rgather_terminate()==0)
	stat=0;
      else
	stat=1;
    } else {
      stat=0;
    }
    CMReturnData(rslt,&stat,CMPI_uint32);   
  } else if (strcasecmp(method,"startsampling") ==0) {
    if( _debug )
      fprintf( stderr, "--- invoking startsampling()\n");
    if (!gs.gsSampling) {
      if (rgather_start()==0)
	stat=1;
      else
	stat=0;
    } else {
      stat=0;
    }
    boolval=stat;
    CMReturnData(rslt,&boolval,CMPI_boolean);   
  } else if (strcasecmp(method,"stopsampling") ==0) {
    if( _debug )
      fprintf( stderr, "--- invoking stopsampling()\n");
    if (gs.gsSampling) {
      if (rgather_stop()==0)
	stat=1;
      else
	stat=0;
    } else {
      stat=0;
    }
    boolval=stat;
    CMReturnData(rslt,&boolval,CMPI_boolean);   
  } else {
    CMSetStatusWithChars( _broker, &st, 
			  CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  }
    
  CMReturnDone(rslt);
  
  return st;
}

/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMInstanceMIStub( OSBase_MetricGathererProvider, 
                  OSBase_MetricGathererProvider, 
                  _broker, 
                  CMNoHook);

CMMethodMIStub( OSBase_MetricGathererProvider, 
		OSBase_MetricGathererProvider, 
		_broker, 
		CMNoHook);

/* ---------------------------------------------------------------------------*/
/*                               Private Stuff                                */
/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*              end of OSBase_MetricGathererProvider                             */
/* ---------------------------------------------------------------------------*/

