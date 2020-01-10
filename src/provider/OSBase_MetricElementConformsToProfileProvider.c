/*
 * $Id: OSBase_MetricElementConformsToProfileProvider.c,v 1.1 2009/07/17 22:15:39 tyreld Exp $
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
 * Description: Provider for SBLIM Gatherer Metric ElementConfromsToProfile
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

CMInstanceMIStub(, OSBase_MetricElementConformsToProfileProvider, _broker, CMNoHook);

/* Association Provider Interface */

static char * _CLASSNAME = "Linux_MetricElementConformsToProfile";

static char * _LEFTREF = "Linux_MetricRegisteredProfile";
static char * _RIGHTREF = "Linux_MetricService";

static char * _LEFTPROP = "ConformantStandard";
static char * _RIGHTROP = "ManagedElement";

static char * _INTEROP = "root/interop";
static char * _CIMV2 = "root/cimv2";

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
							  _CIMV2,
							  _RIGHTREF,
							  NULL);
							  
		assoc = CBEnumInstances(_broker, ctx, cop, NULL, &rc);
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  _INTEROP,
							  _LEFTREF,
							  NULL);
							  
		assoc = CBEnumInstances(_broker, ctx, cop, NULL, &rc);
	} else {
		return rc;
	}
	
	if (CMHasNext(assoc, &rc)) {
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
							  _CIMV2,
							  _RIGHTREF,
							  NULL);
							  
		assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, NULL)) {
		cop = CMNewObjectPath(_broker,
							  _INTEROP,
							  _LEFTREF,
							  NULL);
							  
		assoc = CBEnumInstanceNames(_broker, ctx, cop, &rc);
	} else {
		return rc;
	}
	
	if (CMHasNext(assoc, &rc)) {
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
	CMPIEnumeration * profile;
	CMPIEnumeration * manelm;
	CMPIObjectPath * cop;
	CMPIInstance * inst;
	CMPIData instdata;
	CMPIData enumdata;
	CMPIInstance * rinst;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	instdata.type = CMPI_ref;
	instdata.state = CMPI_goodValue;
	instdata.value.ref = CMGetObjectPath(inst, NULL);
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  _CIMV2,
							  _RIGHTREF,
							  NULL);
							  
		manelm = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(manelm, &rc)) {
			enumdata = CMGetNext(manelm, NULL);
			
			rinst = make_ref_inst(op, instdata, enumdata);
			if (rinst) {
				CMReturnInstance(rslt, rinst);
			}
			rinst = make_ref_inst(cop, instdata, enumdata);
			if (rinst) {
				CMReturnInstance(rslt, rinst);
			}
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  _INTEROP,
							  _LEFTREF,
							  NULL);
							  
		profile = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(profile, &rc)) {
			enumdata = CMGetNext(profile, NULL);
			
			rinst = make_ref_inst(op, enumdata, instdata);
			if (rinst) {
				CMReturnInstance(rslt, rinst);
			}
			rinst = make_ref_inst(cop, enumdata, instdata);
			if (rinst) {
				CMReturnInstance(rslt, rinst);
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
	CMPIEnumeration * profile;
	CMPIEnumeration * manelm;
	CMPIObjectPath * cop;
	CMPIObjectPath * rop;
	CMPIInstance * inst;
	CMPIData instdata;
	CMPIData enumdata;
	CMPIStatus rc = {CMPI_RC_OK, NULL};
	
	inst = CBGetInstance(_broker, ctx, op, NULL, &rc);
	if (rc.rc != CMPI_RC_OK) {
		return rc;
	}
	
	instdata.type = CMPI_ref;
	instdata.state = CMPI_goodValue;
	instdata.value.ref = CMGetObjectPath(inst, NULL);
	
	if (CMClassPathIsA(_broker, op, _LEFTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  _CIMV2,
							  _RIGHTREF,
							  NULL);
							  
		manelm = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(manelm, &rc)) {
			enumdata = CMGetNext(manelm, NULL);
			
			rop = make_ref_path(op, instdata, enumdata);
			if (rop) {
				CMReturnObjectPath(rslt, rop);
			}
			rop = make_ref_path(cop, instdata, enumdata);
			if (rop) {
				CMReturnObjectPath(rslt, rop);
			}
		}
	} else if (CMClassPathIsA(_broker, op, _RIGHTREF, &rc)) {
		cop = CMNewObjectPath(_broker,
							  _INTEROP,
							  _LEFTREF,
							  NULL);
							  
		profile = CBEnumInstanceNames(_broker, ctx, cop, &rc);
		if (rc.rc != CMPI_RC_OK) {
			return rc;
		}
		
		if (CMHasNext(profile, &rc)) {
			enumdata = CMGetNext(profile, NULL);
			
			rop = make_ref_path(op, enumdata, instdata);
			if (rop) {
				CMReturnObjectPath(rslt, rop);
			}
			rop = make_ref_path(cop, enumdata, instdata);
			if (rop) {
				CMReturnObjectPath(rslt, rop);
			}
		}	
	}
	
	CMReturnDone(rslt);
	return rc;
}

CMAssociationMIStub(, OSBase_MetricElementConformsToProfileProvider, _broker, CMNoHook);
