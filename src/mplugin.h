/*
 * $Id: mplugin.h,v 1.2 2009/05/20 19:39:55 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2009
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
 * Description: Metric Plugin Definitions
 *
 */

#ifndef MPLUGIN_H
#define MPLUGIN_H

#include "metric.h"

#define METRIC_DEFINITIONPROC _DefinedMetrics
#define MTERIC_STARTSTOPPROC  _StartStopMetrics 

#define METRIC_DEFINITIONPROC_S "_DefinedMetrics"
#define METRIC_STARTSTOPPROC_S  "_StartStopMetrics"

/* provided by caller */
typedef int (MetricRegisterId) (const char * pluginname, const char *midstr);

/* provided by plugin */
typedef int (MetricsDefined) (MetricRegisterId *mr,
			      const char * pluginname,
			      size_t *mdnum,
			      MetricDefinition **md); 
typedef int (MetricStartStop) (int starting); 


#endif
