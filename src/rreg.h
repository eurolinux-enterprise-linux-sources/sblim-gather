/*
 * $Id: rreg.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2004, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Repository Metric Registration Interface
 *
 */

#ifndef RREG_H
#define RREG_H

#include "metric.h"

#ifdef __cplusplus
extern "C" {
#endif
/* returns unique (process lifetime) metric id for given name */
int RPR_IdForString(const char *pluginname, const char *name);

/* initialize metric processing - optional */
void RPR_InitRegistry();

/* terminate metric processing - removes all metric definitions */
void RPR_FinishRegistry();

/* add new metric definition to registry */
int  RPR_UpdateMetric(const char *pluginname, MetricCalculationDefinition *metric);

/* returns metric definition for id  - this call is efficient */
MetricCalculationDefinition* RPR_GetMetric(int id);

/* delete metric definition for id */
void RPR_RemoveMetric(int id);
 
#ifdef __cplusplus
}
#endif

#endif
