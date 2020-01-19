/*
 * $Id: metriczECKD.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2006, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Alexander Wolf-Reber <a.wolf-reber@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * Metrics Gatherer Plugin for System z ECKD volume metrics
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <time.h>
#include <unistd.h>
#include <commutil.h>
#include "sysfswrapper.h"

#define BUS "ccw"
#define DRIVER "dasd-eckd"

#ifdef DEBUG
#define DBGONLY(X)   X
#else
#define DBGONLY(X) 
#endif


inline int max(int x, int y) {
	return (x>y) ? x : y;
}

inline int min(int x, int y) {
	return (x>y) ? x : y;
}

/* ---------------------------------------------------------------------------*/

static MetricDefinition metricDef[1];

static MetricRetriever  metricRetrECKD;

/* ---------------------------------------------------------------------------*/

int _DefinedMetrics( MetricRegisterId *mr,
		     const char * pluginname,
		     size_t *mdnum,
		     MetricDefinition **md ) {

  DBGONLY(fprintf(stderr,"--- %s(%i) : Retrieving metric definitions\n",
	  __FILE__,__LINE__);)

  if (mr==NULL||mdnum==NULL||md==NULL) {
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
    return -1;
  }

  metricDef[0].mdVersion=MD_VERSION;
  metricDef[0].mdName="_ECKD";
  metricDef[0].mdReposPluginName="librepositoryzECKD.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrECKD;
  metricDef[0].mdeal=free;

  *mdnum=1;
  *md=metricDef;
  return 0;
}

int _StartStopMetrics (int starting) {

  if( starting ) {
  	DBGONLY(fprintf(stderr,"--- %s(%i) : Metric processing starting\n",
	  __FILE__,__LINE__);)
    /* nothing to do */
  }
  else {
  	DBGONLY(fprintf(stderr,"--- %s(%i) : Metric processing stopping\n",
	  __FILE__,__LINE__);)
    /* nothing to do */
  }
  
  return 0;
}

int sendMetric(int mid, MetricReturner mret, sysfsw_Device* device, char* values) {
				   	 
	  MetricValue * mv  = NULL;
	
	  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); return -1; }
	
	  char* resource = sysfsw_getDeviceName(device);
	  
	  mv = calloc(1, sizeof(MetricValue) + 
		         strlen(values) + 1 +
		         strlen(resource) + 1 );
	  if (mv) {
	    mv->mvId = mid;
	    mv->mvTimeStamp = time(NULL);
		mv->mvDataType = MD_STRING;
		mv->mvDataLength = (strlen(values)+1);
		mv->mvData = (char*)mv + sizeof(MetricValue);
		strncpy( mv->mvData, values, strlen(values) );	
	    mv->mvResource = (char*)mv + sizeof(MetricValue) + strlen(values)+1;
	    strcpy(mv->mvResource,resource);
	    mret(mv);
      return 1;
	  }  
  return -1;
}

char* readSysFs(char* devicePath) {
	
	int i, attributeCount;
	size_t strLength;
	
	sysfsw_Device* device = sysfsw_openDevice(devicePath);
	if (device!=NULL) {
		DBGONLY(printf("device opened\n");)
	} else {
		DBGONLY(printf("device open failed\n");)
		return NULL;
	}
	
	const char* CMB_ENABLE = "cmb_enable";
	
	const char* SAMPLE_COUNT = "cmf/sample_count";
	
	const char* ATTRIBUTES[] = { 
							"cmf/avg_control_unit_queuing_time",
							"cmf/avg_device_active_only_time",
							"cmf/avg_device_busy_time",
							"cmf/avg_device_connect_time", 
							"cmf/avg_device_disconnect_time",
							"cmf/avg_function_pending_time",
							"cmf/avg_initial_command_response_time",
							"cmf/avg_sample_interval",
							"cmf/avg_utilization",
							"cmf/ssch_rsch_count",
							"cmf/sample_count",
							NULL};

	for(attributeCount=0; ATTRIBUTES[attributeCount]!=NULL; ++attributeCount);
	
	sysfsw_Attribute* cmbEnableAttr = sysfsw_openAttribute(device, CMB_ENABLE);
	
	if (cmbEnableAttr==NULL) { 
		DBGONLY(printf("cmb_enable could not be opened\n")); 
		sysfsw_closeDevice(device); 
		return NULL; 
	}
	
	int cmbEnable = sysfsw_getAttributeValue(cmbEnableAttr)!=NULL && sysfsw_getAttributeValue(cmbEnableAttr)[0]=='1';
	sysfsw_closeAttribute(cmbEnableAttr);
	
	if (!cmbEnable) { 
		DBGONLY(printf("cmb_enable!=1\n")); 
		sysfsw_closeDevice(device); 
		return NULL; 
	}
	
	sysfsw_Attribute** attributeValues = calloc(attributeCount, sizeof(sysfsw_Attribute*));
	if (attributeValues == NULL) { 
		fprintf(stderr, "calloc() failed\n");
		sysfsw_closeDevice(device);
		return NULL; 
	}

	int sampleCount1, sampleCount2;
	int loops = 0;
	
	do {

		if (loops > 0) {
			for(i=0; i<attributeCount; ++i) {
				sysfsw_closeAttribute(attributeValues[i]);
			}
		}
		
		sysfsw_Attribute* sampleCountAttr = sysfsw_openAttribute(device, SAMPLE_COUNT);
		if (sampleCountAttr==NULL) { 
			DBGONLY(printf("sample_count could not be opened\n")); 
			free(attributeValues);
			sysfsw_closeDevice(device);
			return NULL; 
		}
		
		sampleCount1 = sysfsw_getAttributeValue(sampleCountAttr)!=NULL ? atoi(sysfsw_getAttributeValue(sampleCountAttr)) : -1;
		sysfsw_closeAttribute(sampleCountAttr);

		if (sampleCount1<0)	{ 
			DBGONLY(printf("sample_count <0\n")); 
			free(attributeValues);
			sysfsw_closeDevice(device); 
			return NULL; 
		}

		strLength = 0;
	
		for(i=0; i<attributeCount; ++i) {
			DBGONLY(printf("%s=", ATTRIBUTES[i]);)
			attributeValues[i] = sysfsw_openAttribute(device, ATTRIBUTES[i]);
			if (attributeValues[i]==NULL) { printf("NULL\n"); strLength+=2; continue; }
			strLength += max(strlen(sysfsw_getAttributeValue(attributeValues[i])), 2);
			DBGONLY(printf("%s\n", sysfsw_getAttributeValue(attributeValues[i]));)
		}
		
		sampleCountAttr = attributeValues[attributeCount-1];
		if (sampleCountAttr==NULL) { 
			DBGONLY(printf("sample_count could not be opened\n"));
			for(i=0; i<attributeCount; ++i) {
				sysfsw_closeAttribute(attributeValues[i]);
			}
			free(attributeValues);
			sysfsw_closeDevice(device);
			return NULL; 
		}
		
		sampleCount2 = sysfsw_getAttributeValue(sampleCountAttr)!=NULL ? atoi(sysfsw_getAttributeValue(sampleCountAttr)) : -1;
		
		++loops;
		DBGONLY(printf("sample count = %d/%d, loops = %d\n", sampleCount1, sampleCount2, loops);)
		
	} while (sampleCount2-sampleCount1 && loops<2);

	char* result = calloc(strLength+1, sizeof(char));	

	if (result==NULL) {
		for(i=0; i<attributeCount; ++i) {
			sysfsw_closeAttribute(attributeValues[i]);
		}
		free(attributeValues);
		sysfsw_closeDevice(device);
		return NULL;
	}
	
	size_t remainder = strLength;

	for(i=0; i<attributeCount; ++i) {
		char* value = (attributeValues[i]!=NULL) ? sysfsw_getAttributeValue(attributeValues[i]) : NULL;
		if (value==NULL || strlen(value)==0) { value = "0\n"; }
		strncat(result, value, remainder);
		remainder -= strlen(value);
		sysfsw_closeAttribute(attributeValues[i]);
		result[strlen(result)-1] = ':';
	}

	free(attributeValues);
	sysfsw_closeDevice(device);
	
	return result;
}
	
int metricRetrECKD( int mid, 
			   MetricReturner mret ) { 
			   	
	int deviceCount = 0;
	   	
	sysfsw_Driver* driver = sysfsw_openDriver(BUS, DRIVER);
	if (driver!=NULL) {
		sysfsw_DeviceList* deviceList = sysfsw_readDeviceList(driver);
		if (deviceList!=NULL) {
			sysfsw_startDeviceList(deviceList);
			while (sysfsw_nextDevice(deviceList)) {
				sysfsw_Device* device = sysfsw_getDevice(deviceList);
				DBGONLY(printf("device name = %s\n", sysfsw_getDeviceName(device));)
				char* values = readSysFs(sysfsw_getDevicePath(device));
				if (values!=NULL) {
					DBGONLY(printf("%s\n", values);)
					int result = sendMetric(mid, mret, device, values); 
					free(values);
					if (!result) { deviceCount = -1; break; }
					++deviceCount;
				}
			}
		}
		sysfsw_closeDriver(driver);
	}
	
	return deviceCount;
}
/* ---------------------------------------------------------------------------*/
/*                          end of metricProcessor.c                          */
/* ---------------------------------------------------------------------------*/

