/*
 * $Id: OSBase_MetricRepositoryServiceProvider.c,v 1.13 2012/10/25 23:13:52 tyreld Exp $
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
 * Contributors:
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: Provider for the SBLIM RepositoryService.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <metric.h>
#include <rrepos.h>

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

static char * _ClassName = "Linux_MetricRepositoryService";
static char * _Name = "reposd";
static char * _FILENAME = "OSBase_MetricRepositoryServiceProvider.c";

static char  _false=0;

#ifdef CMPI_VER_100
#define OSBase_MetricRepositoryServiceProviderSetInstance OSBase_MetricRepositoryServiceProviderModifyInstance 
#endif

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_MetricRepositoryServiceProviderCleanup( CMPIInstanceMI * mi, 
							  const CMPIContext * ctx, 
							  CMPIBoolean terminate) { 
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI Cleanup()\n", _FILENAME, _ClassName );
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricRepositoryServiceProviderEnumInstanceNames( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricRepositoryServiceProviderEnumInstances( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) { 
  CMPIObjectPath * op = NULL;
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  RepositoryStatus   rs;
  short          ival;
  short          nump;
  short          numm;
  char           boolval;
  size_t         limit;
  int            ascending;
    
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

    if (rrepos_status(&rs)) {
      CMSetProperty(ci,"Started",&_false,CMPI_boolean);
      ival=0; /* unknown */
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
    } else {
      boolval=rs.rsInitialized;
      CMSetProperty(ci,"Started",&boolval,CMPI_boolean);
      if (rs.rsInitialized)
	ival=2;
      else
	ival=0;
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
      nump = rs.rsNumPlugins; 
      CMSetProperty(ci,"NumberOfPlugins",&nump,CMPI_uint16);
      numm = rs.rsNumMetrics;
      CMSetProperty(ci,"NumberOfMetrics",&numm,CMPI_uint16);
    }
    if (rrepos_getglobalfilter(&limit,&ascending)==0) {
      CMSetProperty(ci,"EnumerationLimit",&limit,CMPI_uint32);
      boolval=ascending?1:0;
      CMSetProperty(ci,"Ascending",&boolval,CMPI_boolean);
    }
    CMReturnInstance(rslt,ci);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not build object path" ); 
  }
  CMReturnDone( rslt );
  return rc;
}

CMPIStatus OSBase_MetricRepositoryServiceProviderGetInstance( CMPIInstanceMI * mi, 
           const CMPIContext * ctx, 
           const CMPIResult * rslt, 
           const CMPIObjectPath * ref, 
           const char ** properties) {
  CMPIObjectPath * op = NULL;
  CMPIInstance * ci = NULL;
  CMPIStatus     rc = {CMPI_RC_OK, NULL};
  RepositoryStatus   rs;
  short          ival;
  short          nump;
  short          numm;
  char           boolval;
  size_t         limit;
  int            ascending;

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

    if (rrepos_status(&rs)) {
      CMSetProperty(ci,"Started",&_false,CMPI_boolean);
      ival=0; /* unknown */
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
    } else {
      boolval=rs.rsInitialized;
      CMSetProperty(ci,"Started",&boolval,CMPI_boolean);
      if (rs.rsInitialized)
	ival=2;
      else
	ival=0;
      CMSetProperty(ci,"EnabledState",&ival,CMPI_uint16);
      nump = rs.rsNumPlugins;
      CMSetProperty(ci,"NumberOfPlugins",&nump,CMPI_uint16);
      numm = rs.rsNumMetrics;
      CMSetProperty(ci,"NumberOfMetrics",&numm,CMPI_uint16);
      if (rrepos_getglobalfilter(&limit,&ascending)==0) {
	CMSetProperty(ci,"EnumerationLimit",&limit,CMPI_uint32);
	boolval=ascending?1:0;
	CMSetProperty(ci,"Ascending",&boolval,CMPI_boolean);
      }
    }
    CMReturnInstance(rslt,ci);
  } else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "RepositoryService Service not active" ); 
  }
  CMReturnDone(rslt);
  return rc;
}

CMPIStatus OSBase_MetricRepositoryServiceProviderCreateInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricRepositoryServiceProviderSetInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricRepositoryServiceProviderDeleteInstance( CMPIInstanceMI * mi, 
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

CMPIStatus OSBase_MetricRepositoryServiceProviderExecQuery( CMPIInstanceMI * mi, 
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
CMPIStatus OSBase_MetricRepositoryServiceProviderMethodCleanup( CMPIMethodMI * mi, 
								const CMPIContext * ctx, 
								CMPIBoolean terminate ) 
{
  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI MethodCleanup()\n", _FILENAME, _ClassName );

  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_MetricRepositoryServiceProviderInvokeMethod( CMPIMethodMI * mi, 
					      const CMPIContext * ctx, 
					      const CMPIResult * rslt,
					      const CMPIObjectPath * cop,
					      const char * method,
					      const CMPIArgs * in,
					      CMPIArgs * out)
{
  CMPIStatus   st = {CMPI_RC_OK,NULL};
  RepositoryStatus rs;
  CMPIData     data;
  CMPIEnumeration * en;
  CMPIObjectPath  * copPlugin;
  int          stat;
  char         boolval, ascending;
  unsigned     limit;

  if( _debug )
    fprintf( stderr, "--- %s : %s CMPI InvokeMethod()\n", _FILENAME, _ClassName );

  if (rrepos_status(&rs)) {
    rs.rsInitialized=0;
  }

  if (strcasecmp(method,"startservice")==0) {
    if( _debug )
      fprintf( stderr, "--- invoking startservice()\n");
    if (!rs.rsInitialized) {
      rrepos_load(); /* attempt to start the daemon - just in case */
      if (rrepos_init()==0) {
	if( _debug )
	  fprintf( stderr, "--- attempting init\n");
	stat=0;
	/* now load plugins */
	copPlugin = CMNewObjectPath(_broker,
				    CMGetCharPtr(CMGetNameSpace(cop,NULL)),
				    "Linux_RepositoryPlugin",NULL);
	if (copPlugin) {
	  if( _debug )
	    fprintf( stderr, "--- searching plugins\n");
	  en = CBEnumInstances(_broker,ctx,copPlugin,NULL,NULL);
	  while (CMHasNext(en,NULL)) {
	    data = CMGetNext(en,NULL);
	    if (data.value.inst) {
	      data=CMGetProperty(data.value.inst,"RepositoryPluginName",NULL);
	      if (data.type == CMPI_string && data.value.string) {
		if( _debug )
		  fprintf( stderr, "--- adding plugin %s\n",
			   CMGetCharPtr(data.value.string));
		rreposplugin_add(CMGetCharPtr(data.value.string));
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
    if (rs.rsInitialized) {
      if (rrepos_terminate()==0)
	stat=0;
      else
	stat=1;
    } else {
      stat=0;
    }
    CMReturnData(rslt,&stat,CMPI_uint32);   
  } else if (strcasecmp(method,"setenumerationlimits") ==0) {
    if( _debug )
      fprintf( stderr, "--- invoking setenumerationlimits()\n");
    boolval = 0;
    data = CMGetArg(in,"limit",&st);
    if (st.rc == CMPI_RC_OK) {
      limit = data.value.uint32;
      data = CMGetArg(in,"ascending",&st);
      if (st.rc == CMPI_RC_OK) {
	ascending = data.value.boolean;
	if( _debug )
	  fprintf( stderr, "--- setting filter %d %d\n", limit, ascending);
	if (rrepos_setglobalfilter(limit,ascending)==0) {
	  boolval = 1;
	}
      }
    }
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

CMInstanceMIStub( OSBase_MetricRepositoryServiceProvider, 
                  OSBase_MetricRepositoryServiceProvider, 
                  _broker, 
                  CMNoHook);

CMMethodMIStub( OSBase_MetricRepositoryServiceProvider, 
		OSBase_MetricRepositoryServiceProvider, 
		_broker, 
		CMNoHook);

/* ---------------------------------------------------------------------------*/
/*                               Private Stuff                                */
/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*              end of OSBase_MetricRepositoryServiceProvider                 */
/* ---------------------------------------------------------------------------*/

