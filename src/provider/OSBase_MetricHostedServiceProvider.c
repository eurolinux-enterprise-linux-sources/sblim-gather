/*
 * $Id: OSBase_MetricHostedServiceProvider.c,v 1.3 2012/10/25 23:13:52 tyreld Exp $
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
 * Description: Provider for SBLIM Gatherer Metric Hosted Service
 *
 */

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include <stdio.h>

const CMPIBroker * _broker;

/* Instance Provider Interface */

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
	CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus EnumInstances(CMPIInstanceMI * mi,
                                const CMPIContext * ctx,
                                const CMPIResult * rslt,
                                const CMPIObjectPath * op, 
                                const char **props)
{
	CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus GetInstance(CMPIInstanceMI * mi,
                              const CMPIContext * ctx,
                              const CMPIResult * rslt,
                              const CMPIObjectPath * op, 
                              const char **props)
{
 	CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
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

CMInstanceMIStub(, OSBase_MetricHostedServiceProvider, _broker, CMNoHook);

/* Association Provider Interface */

static char * _CLASSNAME = "Linux_MetricHostedService";

static char * _LEFTREF = "Linux_ComputerSystem";
static char * _RIGHTREF = "Linux_MetricService";

static char * _LEFTPROP = "Antecedent";
static char * _RIGHTROP = "Dependent";

static CMPIInstance * make_ref_inst(const CMPIObjectPath * op,
					CMPIData cap,
					CMPIData manelm,
                                        const char ** props)
{
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
	if (cop) {
		inst = CMNewInstance(_broker, cop, NULL);
		
                if (inst) {
                    CMSetPropertyFilter(inst, props, NULL);
		    CMSetProperty(inst, _LEFTPROP, &cap.value, CMPI_ref);
		    CMSetProperty(inst, _RIGHTROP, &manelm.value, CMPI_ref);
		    return inst;
                }
	}
	
	return NULL;
}

static CMPIObjectPath * make_ref_path(const CMPIObjectPath * op,
									  CMPIData cap,
									  CMPIData manelm)
{
	CMPIObjectPath * cop;
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
						  
	if (cop) {
		CMAddKey(cop, _LEFTPROP, &cap.value, CMPI_ref);
		CMAddKey(cop, _RIGHTROP, &manelm.value, CMPI_ref);
		
		return cop;
	}
	
	return NULL;
}

static CMPIStatus AssociationCleanup(CMPIAssociationMI * mi,
									 const CMPIContext * ctx,
									 CMPIBoolean terminating)
{
	CMReturn(CMPI_RC_OK);
}

static CMPIStatus Associators(CMPIAssociationMI * mi,
							  const CMPIContext * ctx,
							  const CMPIResult * rslt,
							  const CMPIObjectPath * op,
							  const char * assocClass,
							  const char * resultClass,
							  const char * role,
							  const char * resultRole,
							  const char ** properties)
{
	CMPIEnumeration * assoc = NULL;
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	CMPIData data;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _RIGHTREF,
							  NULL);
		if (resultClass && CMClassPathIsA(_broker, cop, resultClass, &rc)) {
			assoc = CBEnumInstances(_broker, ctx, cop, properties, &rc);
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
		if (resultClass && CMClassPathIsA(_broker, cop, resultClass, &rc)) {
			assoc = CBEnumInstances(_broker, ctx, cop, properties, &rc);
		}
	} else {
		return rc;
	}
	
	if (assoc && CMHasNext(assoc, &rc)) {
		data = CMGetNext(assoc, NULL);
		CMReturnInstance(rslt, data.value.inst);
	} else {
		return rc;
	}
	
	CMReturnDone(rslt);
	return rc;
}

static CMPIStatus AssociatorNames(CMPIAssociationMI * mi,
								   const CMPIContext * ctx,
								   const CMPIResult * rslt,
								   const CMPIObjectPath * op,
								   const char * assocClass,
								   const char * resultClass,
								   const char * role,
								   const char * resultRole)
{
	CMPIEnumeration * assoc = NULL;
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	CMPIData data;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _RIGHTREF,
							  NULL);
		if (CMClassPathIsA(_broker, cop, resultClass, &rc)) { 
			assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
		if (CMClassPathIsA(_broker, cop, resultClass, &rc)) {
			assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		}
	} else {
		return rc;
	}
	
	if (assoc && CMHasNext(assoc, &rc)) {
		data = CMGetNext(assoc, NULL);
		CMReturnObjectPath(rslt, data.value.ref);
	} else {
		return rc;
	}
	
	CMReturnDone(rslt);
	return rc;
}

static CMPIStatus References(CMPIAssociationMI * mi,
							 const CMPIContext * ctx,
							 const CMPIResult * rslt,
							 const CMPIObjectPath * op,
							 const char * role,
							 const char * roleResult,
							 const char ** properties)
{
	CMPIEnumeration * cap;
	CMPIEnumeration * manelm;
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc) ||
		CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		
		inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		cop = CMNewObjectPath(_broker, 
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
	
		cap = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _RIGHTREF,
							  NULL);
						  
		manelm = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		if (CMHasNext(cap, &rc) && CMHasNext(manelm, &rc)) {
			inst = make_ref_inst(op, CMGetNext(cap, NULL), CMGetNext(manelm, NULL), properties);
			if (inst) {
				CMReturnInstance(rslt, inst);
			}
		}
	}
	
	CMReturnDone(rslt);
	return rc;
}

static CMPIStatus ReferenceNames(CMPIAssociationMI * mi,
								 const CMPIContext * ctx,
								 const CMPIResult * rslt,
								 const CMPIObjectPath * op,
								 const char * role,
								 const char * resultRole)
{
	CMPIEnumeration * cap;
	CMPIEnumeration * manelm;
	CMPIInstance * inst;
	CMPIObjectPath * cop;
	CMPIObjectPath * path;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc) ||
		CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		
		inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		cop = CMNewObjectPath(_broker, 
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
	
		cap = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}	
	
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _RIGHTREF,
							  NULL);
						  
		manelm = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		if (CMHasNext(cap, &rc) && CMHasNext(manelm, &rc)) {
			path = make_ref_path(op, CMGetNext(cap, NULL), CMGetNext(manelm, NULL));
			if (path) {
				CMReturnObjectPath(rslt, path);
			}
		}
	}
	
	CMReturnDone(rslt);
	return rc;
}

CMAssociationMIStub(, OSBase_MetricHostedServiceProvider, _broker, CMNoHook);
