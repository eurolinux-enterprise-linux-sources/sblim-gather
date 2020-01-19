/*
 * $Id: OSBase_MetricServiceCapabilitiesProvider.c,v 1.4 2012/10/25 23:13:52 tyreld Exp $
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
 * Description: Provider for the SBLIM Gatherer Metric Service Capabilities
 *
 */

#include <string.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

const CMPIBroker * _broker;

static char * _CLASSNAME = "Linux_MetricServiceCapabilities";
static char * _INSTANCEID = "SBLIM:MetricServiceCapabilities";
/* static char * _FILENAME = "OSBase_MetricServiceCapabilitiesProvider.c"; */

static void check_keys(const CMPIObjectPath * op, CMPIStatus * rc)
{
	CMPIString * key;
	
	key = CMGetKey(op, "InstanceID", rc).value.string;
	
	if (rc->rc != CMPI_RC_OK || key == NULL) {
		CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_FAILED,
							 "Couldn't get value for InstanceID");
		return;
	}
	
	if (strcasecmp(CMGetCharPtr(key), _INSTANCEID) != 0) {
		CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_NOT_FOUND,
							 "Instance doesn't exist");
		return;
	}
	
	return;
}

static CMPIObjectPath * make_path(const CMPIObjectPath * op)
{
	CMPIObjectPath * cop;
	cop = CMNewObjectPath(_broker, 
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
						 
	if (cop) {
		CMAddKey(cop, "InstanceID", _INSTANCEID, CMPI_chars);
		return cop;
	}
	
	return NULL;
}

static CMPIInstance * make_inst(const CMPIObjectPath * op, const char **props)
{
	CMPIObjectPath * cop;
	CMPIInstance * ci = NULL;
    CMPIArray * array_uint = NULL;
    CMPIArray * array_string = NULL;
	
	cop = CMNewObjectPath(_broker,
			  CMGetCharPtr(CMGetNameSpace(op, NULL)),
			  _CLASSNAME,
			  NULL);
						  
	if (cop) {
	    ci = CMNewInstance(_broker, cop, NULL);
	}
	
	if (ci) {
            CMSetPropertyFilter(ci, props, NULL);
            CMSetProperty(ci, "InstanceID", _INSTANCEID, CMPI_chars);

		CMSetProperty(ci, "ElementName", _CLASSNAME, CMPI_chars);

        array_string = CMNewArray(_broker, 0, CMPI_string, NULL);
        CMSetProperty(ci, "ControllableManagedElements", (CMPIValue *)
                &array_string, CMPI_stringA);
        CMSetProperty(ci, "ControllableMetrics", (CMPIValue *) &array_string,
                CMPI_stringA);

        array_uint = CMNewArray(_broker, 0, CMPI_uint16, NULL);
        CMSetProperty(ci, "ManagedElementControlTypes", (CMPIValue *)
                &array_uint, CMPI_uint16A);
        CMSetProperty(ci, "MetricsControlTypes", (CMPIValue *) &array_uint,
                CMPI_uint16A);
        CMSetProperty(ci, "SupportedMethods", (CMPIValue *) &array_uint, CMPI_uint16A);
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
    CMPIInstance * ci;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

 	ci = make_inst(op, props);

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
    CMPIInstance * ci;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

	check_keys(op, &rc);
	
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	ci = make_inst(op, props);

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

CMInstanceMIStub(, OSBase_MetricServiceCapabilitiesProvider, _broker, CMNoHook);

