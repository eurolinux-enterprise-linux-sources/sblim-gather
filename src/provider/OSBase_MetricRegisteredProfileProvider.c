/*
 * $Id: OSBase_MetricRegisteredProfileProvider.c,v 1.4 2012/10/25 23:13:52 tyreld Exp $
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
 * Description: Provider for SBLIM Metric Gatherer RegisterdProfile
 *
 */

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include <string.h>

const CMPIBroker * _broker;

static const char * _CLASSNAME = "Linux_MetricRegisteredProfile";
static const char * _INSTANCEID = "CIM:DSP1053-BaseMetrics-1.0.1";

static const char * _VERSION = "1.0.1";

/* Instance Provider Interface */

static void check_keys(const CMPIObjectPath * op, CMPIStatus * rc)
{
	CMPIString * key;
	
	key = CMGetKey(op, "InstanceId", rc).value.string;
		
	if (rc->rc != CMPI_RC_OK) {
		return;
	} else if ( key == NULL) {
		CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_FAILED,
							 "NULL key value");
	}
		
	if (strcasecmp(CMGetCharPtr(key), _INSTANCEID) != 0) {
		CMSetStatusWithChars(_broker, rc, CMPI_RC_ERR_NOT_FOUND,
							 "The requested instance doesn't exist");
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
		CMAddKey(cop, "InstanceId", _INSTANCEID, CMPI_chars);
		return cop;
	}
	
	return NULL;
}

static CMPIInstance * make_inst(const CMPIObjectPath * op, const char ** props)
{
	CMPIObjectPath * cop;
	CMPIInstance * ci = NULL;
	CMPIArray * array;
	unsigned short regorg = 2;	/* DMTF */
	unsigned short adtype = 3;	/* SLP */
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
						  
	if (cop) {
		ci = CMNewInstance(_broker, cop, NULL);
	}
	
	if (ci) {
        CMSetPropertyFilter(ci, props, NULL);
	CMSetProperty(ci, "InstanceId", _INSTANCEID, CMPI_chars);
		
        CMSetProperty(ci, "RegisteredOrganization", 
                      &regorg, CMPI_uint16);

        CMSetProperty(ci, "RegisteredName", 
                      "Base Metrics", CMPI_chars);

        CMSetProperty(ci, "RegisteredVersion", 
                      _VERSION, CMPI_chars);

        array = CMNewArray(_broker, 1, CMPI_uint16, NULL);
        if (CMIsNullObject(array)) {
                return NULL;
        }

        CMSetArrayElementAt(array, 0, &adtype, CMPI_uint16);

        CMSetProperty(ci, "AdvertiseTypes",
                      (CMPIValue *)&array, CMPI_uint16A);

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
    CMPIInstance * ci = NULL;
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

CMInstanceMIStub(, OSBase_MetricRegisteredProfileProvider, _broker, CMNoHook);
