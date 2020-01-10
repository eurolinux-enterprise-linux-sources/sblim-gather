/*
 * $Id: OSBase_MetricServiceProvider.c,v 1.3 2010/05/22 02:07:51 tyreld Exp $
 *
 * © Copyright IBM Corp. 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Tyrel Datwyler <tyreld@us.ibm.com>
 * Contributors:
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: Provider for the SBLIM Gatherer Metric Service
 *
 */

#include <string.h>

#include <metric.h>

#include <OSBase_Common.h>
#include <cmpiOSBase_Common.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

const CMPIBroker * _broker;

enum METHOD_RETURN {
	METHOD_SUCCESS = 0,
	METHOD_NOT_SUPPORTED = 1,
	METHOD_FAILED = 2,
};

#define NUMKEYS 4

struct keypair {
	const char * key;
	const char * value;
};

static struct keypair keys[4];

static const char * _CLASSNAME = "Linux_MetricService";
static const char * _NAME = "SBLIM Metric Data Gatherer";
static const char * _FILENAME = "OSBase_MetricServiceProvider.c";

static void init_keys()
{
	keys[0] = (struct keypair) { "CreationClassName", _CLASSNAME };
	keys[1] = (struct keypair) { "Name", _NAME };
	keys[2] = (struct keypair) { "SystemCreationClassName", CSCreationClassName };
	keys[3] = (struct keypair) { "SystemName", get_system_name() };
	
	return;
}

static void check_keys(const CMPIObjectPath * op, CMPIStatus * rc)
{
	CMPIString * key;
	int i;
	
	for (i = 0; i < NUMKEYS; i++) {
		key = CMGetKey(op, keys[i].key, rc).value.string;
		
		if (rc->rc != CMPI_RC_OK) {
			return;
		} else if ( key == NULL) {
			CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_FAILED,
								 "NULL key value");
		}
		
		if (strcasecmp(CMGetCharPtr(key), keys[i].value) != 0) {
			CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_NOT_FOUND,
								 "The requested instance doesn't exist");
			return;
		}
	}
	
	return;
}

static CMPIObjectPath * make_path(const CMPIObjectPath * op)
{
	CMPIObjectPath * cop;
	int i;
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
	
	if (cop) {
		for (i = 0; i < NUMKEYS; i++) {
			CMAddKey(cop, keys[i].key, keys[i].value, CMPI_chars);
		}
		return cop;
	}
	
	return NULL;
}

static CMPIInstance * make_inst(const CMPIObjectPath * op)
{
	CMPIObjectPath * cop;
	CMPIInstance * ci = NULL;
	int i;
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
						  
	if (cop) {
		ci = CMNewInstance(_broker, cop, NULL);
	}
	
	if (ci) {
		for (i = 0; i < NUMKEYS; i++) {
			CMSetProperty(ci, keys[i].key, keys[i].value, CMPI_chars);
		}
		CMSetProperty(ci, "ElementName", _CLASSNAME, CMPI_chars);
        CMSetProperty(ci, "Release", PACKAGE_VERSION, CMPI_chars);
		return ci;
	}
	
	return NULL;
}

static CMPIStatus Cleanup(CMPIInstanceMI * mi,
                          const CMPIContext * ctx, 
                          CMPIBoolean terminating)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus EnumInstanceNames(CMPIInstanceMI * mi,
                                    const CMPIContext * ctx,
                                    const CMPIResult * rslt,
                                    const CMPIObjectPath * op)
{
    CMPIObjectPath * cop;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

    cop = make_path(op);

    if (cop) {
        CMReturnObjectPath(rslt, cop);
    } else {
        CMSetStatusWithChars(_broker, &rc, CMPI_RC_ERR_FAILED,
                             "Couldn't build objectpath");
    }

    CMReturnDone(rslt);
    return rc;
}

static CMPIStatus EnumInstances(CMPIInstanceMI * mi,
                                const CMPIContext * ctx,
                                const CMPIResult * rslt,
                                const CMPIObjectPath * op, 
                                const char **props)
{
    CMPIInstance * ci = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

	ci = make_inst(op);
	
	if (ci) {
		CMReturnInstance(rslt, ci);
	} else {
		CMSetStatusWithChars(_broker, &rc, CMPI_RC_ERR_FAILED,
							 "Couldn't build instance");
	}

    CMReturnDone(rslt);
    return rc;
}

static CMPIStatus GetInstance(CMPIInstanceMI * mi,
                              const CMPIContext * ctx,
                              const CMPIResult * rslt,
                              const CMPIObjectPath * op, 
                              const char **props)
{
    CMPIInstance * ci = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

	check_keys(op, &rc);
	
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}

	ci = make_inst(op);
	
    if (ci) {
        CMReturnInstance(rslt, ci);
    } else {
        CMSetStatusWithChars(_broker, &rc, CMPI_RC_ERR_FAILED,
                             "Couldn't build instance");
    }

    CMReturnDone(rslt);
    return rc;
}

static CMPIStatus CreateInstance(CMPIInstanceMI * mi,
                                 const CMPIContext * ctx,
                                 const CMPIResult * rslt,
                                 const CMPIObjectPath * op,
                                 const CMPIInstance * inst)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus ModifyInstance(CMPIInstanceMI * mi,
                                 const CMPIContext * ctx,
                                 const CMPIResult * rslt,
                                 const CMPIObjectPath * op,
                                 const CMPIInstance * inst, 
                                 const char **props)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus DeleteInstance(CMPIInstanceMI * mi,
                                 const CMPIContext * ctx,
                                 const CMPIResult * rslt,
                                 const CMPIObjectPath * op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus ExecQuery(CMPIInstanceMI * mi,
                            const CMPIContext * ctx,
                            const CMPIResult * rslt,
                            const CMPIObjectPath * op,
                            const char *query, 
                            const char *lang)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(, OSBase_MetricServiceProvider, _broker, init_keys());

static CMPIStatus MethodCleanup(CMPIMethodMI * mi,
                                const CMPIContext * ctx,
                                CMPIBoolean terminating)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus InvokeMethod(CMPIMethodMI * mi,
                               const CMPIContext * ctx,
                               const CMPIResult * rslt,
                               const CMPIObjectPath * op,
                               const char *method,
                               const CMPIArgs * in, 
                               CMPIArgs * out)
{
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	CMPIValue valrc;

	check_keys(op, &rc);
	
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	if (strcasecmp(method, "ShowMetrics") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "ShowMetricsByClass") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);	
	} else if (strcasecmp(method, "ControlMetrics") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "ControlMetricsByClass") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);	
	} else if (strcasecmp(method, "GetMetricValues") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "StartService") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "StopService") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "ChangeAffectedElementsAssignedSequence") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else if (strcasecmp(method, "RequestStateChange") == 0) {
		valrc.uint32 = METHOD_NOT_SUPPORTED;
		CMReturnData(rslt, &valrc, CMPI_uint32);
		CMReturnDone(rslt);
	} else {
		CMSetStatusWithChars(_broker, &rc, CMPI_RC_ERR_METHOD_NOT_FOUND, method);	
	}
	
    return rc;
}

CMMethodMIStub(, OSBase_MetricServiceProvider, _broker, CMNoHook);

