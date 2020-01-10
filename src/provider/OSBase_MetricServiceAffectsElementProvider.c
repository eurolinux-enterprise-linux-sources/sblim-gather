/*
 * $Id: OSBase_MetricServiceAffectsElementProvider.c,v 1.2 2009/07/17 22:17:18 tyreld Exp $
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
 * Description: Provider for SBLIM Gatherer Metric ServiceAffectsElement
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

CMInstanceMIStub(, OSBase_MetricServiceAffectsElementProvider, _broker, CMNoHook);

/* Association Provider Interface */

static char * _CLASSNAME = "Linux_MetricServiceAffectsElement";

static char * _LEFTREF = "CIM_BaseMetricDefinition";
static char * _RIGHTREF = "Linux_MetricService";

static char * _LEFTPROP = "AffectedElement";
static char * _RIGHTROP = "AffectingElement";

static CMPIInstance * make_ref_inst(const CMPIObjectPath * op,
									CMPIData cap,
									CMPIData manelm)
{
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	
	cop = CMNewObjectPath(_broker,
						  CMGetCharPtr(CMGetNameSpace(op, NULL)),
						  _CLASSNAME,
						  NULL);
	if (cop) {
		inst = CMNewInstance(_broker, cop, NULL);
		CMSetProperty(inst, _LEFTPROP, &cap.value, CMPI_ref);
		CMSetProperty(inst, _RIGHTROP, &manelm.value, CMPI_ref);
		
		return inst;
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
	CMPIEnumeration * assoc;
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
							  
		assoc = CBEnumInstances(_broker, ctx, cop, NULL, &rc);
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
							  
		assoc = CBEnumInstances(_broker, ctx, cop, NULL, &rc);
	} else {
		return rc;
	}
	
	if (CMHasNext(assoc, &rc)) {
		while(CMHasNext(assoc, &rc)) {
			data = CMGetNext(assoc, NULL);
			CMReturnInstance(rslt, data.value.inst);
		}
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
	CMPIEnumeration * assoc;
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
							  
		assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
							  
		assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
	} else {
		return rc;
	}
	
	if (CMHasNext(assoc, &rc)) {
		while (CMHasNext(assoc, &rc)) {
			data = CMGetNext(assoc, NULL);
			CMReturnObjectPath(rslt, data.value.ref);
		}
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
	CMPIEnumeration * service;
	CMPIEnumeration * metdef;
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	CMPIInstance * rinst;
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
							  
		service = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		if (CMHasNext(service, &rc)) {
			data.type = CMPI_ref;
			data.state = CMPI_goodValue;
			data.value.ref = CMGetObjectPath(inst, NULL);
			
			rinst = make_ref_inst(op, data, CMGetNext(service, NULL));
			
			if (rinst) {
				CMReturnInstance(rslt, rinst);
			}
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
							  
		metdef = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(metdef, &rc)) {
			data.type = CMPI_ref;
			data.state = CMPI_goodValue;
			data.value.ref = CMGetObjectPath(inst, NULL);
			
			while (CMHasNext(metdef, &rc)) {
				rinst = make_ref_inst(op, CMGetNext(metdef, NULL), data);
				
				if (rinst) {
					CMReturnInstance(rslt, rinst);
				}
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
	CMPIEnumeration * service;
	CMPIEnumeration * metdef;
	CMPIInstance * inst;
	CMPIObjectPath * cop;
	CMPIObjectPath * path;
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
							  
		service = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
	
		if (CMHasNext(service, &rc)) {
			data.type = CMPI_ref;
			data.state = CMPI_goodValue;
			data.value.ref = CMGetObjectPath(inst, NULL);
			
			path = make_ref_path(op, data, CMGetNext(service, NULL));
			
			if (path) {
				CMReturnObjectPath(rslt, path);
			}
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  CMGetCharPtr(CMGetNameSpace(op, NULL)),
							  _LEFTREF,
							  NULL);
							  
		metdef = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(metdef, &rc)) {
			data.type = CMPI_ref;
			data.state = CMPI_goodValue;
			data.value.ref = CMGetObjectPath(inst, NULL);
			
			while (CMHasNext(metdef, &rc)) {
				path = make_ref_path(op, CMGetNext(metdef, NULL), data);
				
				if (path) {
					CMReturnObjectPath(rslt, path);
				}
			}
		}
	}

	CMReturnDone(rslt);
	return rc;
}

CMAssociationMIStub(, OSBase_MetricServiceAffectsElementProvider, _broker, CMNoHook);
