/*
 * $Id: metriczCH.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Metrics Gatherer Plugin for System z channel metrics
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
#include <pthread.h>
#include "sysfswrapper.h"
#include "channelutil.h"

#ifdef DEBUG
#define DBGONLY(X)   X
#else
#define DBGONLY(X) 
#endif

#define MAX_PATH_LENGTH 255
#define MAX_CHPID_COUNT 256

inline int max(int x, int y) {
	return (x>y) ? x : y;
}

inline int min(int x, int y) {
	return (x>y) ? x : y;
}

const int true = (1==1);
const int false = (1==0);

/* ---------------------------------------------------------------------------*/

static MetricDefinition metricDef[1];

static MetricRetriever  metricRetrCUE;

static pthread_mutex_t mutex;
static char* deviceList;
static size_t deviceCount;

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
  metricDef[0].mdName="_CUE_CMC";
  metricDef[0].mdReposPluginName="librepositoryzCH.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrCUE;
  metricDef[0].mdeal=free;

  *mdnum=1;
  *md=metricDef;
  return 0;
}

int _StartStopMetrics (int starting) {

  if( starting ) {
  	DBGONLY(fprintf(stderr,"--- %s(%i) : Metric processing starting\n",
	  __FILE__,__LINE__);)
	deviceCount = 0;
    deviceList = calloc(MAX_CHPID_COUNT, MAX_PATH_LENGTH+1);
    pthread_mutex_init(&mutex, NULL);
  }
  else {
  	DBGONLY(fprintf(stderr,"--- %s(%i) : Metric processing stopping\n",
	  __FILE__,__LINE__);)
    deviceCount = 0;
    free(deviceList);
    deviceList = NULL;
    pthread_mutex_destroy(&mutex);
  }
  
  return 0;
}

int sendMetric(int mid, MetricReturner mret, char* resourcepath, char* cue_values, char* cmc_values) {
				   	 
	  MetricValue * mv  = NULL;
	  
	  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); return -1; }
	
	  char* resource = strrchr(resourcepath, '/') + 1;
	  
	  mv = calloc(1, sizeof(MetricValue) + 
		         strlen(cue_values) + 1 +
		         strlen(cmc_values) + 1 +
		         strlen(resource) + 1 );
	  if (mv) {
	    mv->mvId = mid;
	    mv->mvTimeStamp = time(NULL);
		mv->mvDataType = MD_STRING;
		mv->mvDataLength = strlen(cue_values) + 1 + strlen(cmc_values) + 1;
		mv->mvData = (char*)mv + sizeof(MetricValue);
		strcpy( mv->mvData, cue_values);	
		strcat( mv->mvData, "!");
		strcat( mv->mvData, cmc_values);
	    mv->mvResource = (char*)mv + sizeof(MetricValue) + strlen(cue_values) + 1 + strlen(cmc_values) + 1;
	    strcpy(mv->mvResource,resource);
	    mret(mv);
      return 1;
	  }  
  return -1;
}

zChannelUtilization* readCue(char* devicePath, zChannelUtilization* cu) {
	if (devicePath==NULL || cu==NULL) return NULL;
	sysfsw_Attribute* measAttr = sysfsw_openAttributePath(devicePath, "measurement");
	if (measAttr==NULL) return NULL;
	
	char* measVal = sysfsw_getAttributeValue(measAttr);
	parseCue(2, measVal, cu);
	sysfsw_closeAttribute(measAttr);

	return cu;
}

zChannelMeasurementCharacteristics* readCmc(char* devicePath, zChannelMeasurementCharacteristics* cmc) {
	if (devicePath==NULL || cmc==NULL) return NULL;
	sysfsw_Attribute* mcAttr = sysfsw_openAttributePath(devicePath, "measurement_chars");
	if (mcAttr==NULL) return NULL;
	
	char* mcVal = sysfsw_getAttributeValue(mcAttr);
	parseCmc(2, mcVal, cmc);
	sysfsw_closeAttribute(mcAttr);

	return cmc;
}

void updateDeviceList(sysfsw_Device* device, size_t pos) {
	if (pos<MAX_CHPID_COUNT) {
		strncpy(deviceList+pos*(MAX_PATH_LENGTH+1), sysfsw_getDevicePath(device), MAX_PATH_LENGTH);
	}
}

int filterDevices(sysfsw_Device* device) {
	sysfsw_Attribute* cmgAttr = sysfsw_openAttribute(device, "cmg");
	if (cmgAttr==NULL) return false;
	
	char* cmgVal = sysfsw_getAttributeValue(cmgAttr);
	int cmg = atoi(cmgVal);
	sysfsw_closeAttribute(cmgAttr);
	
	return (cmg == 2);
}

int refreshDeviceList() {
	if (pthread_mutex_lock(&mutex)==0) {
		if (deviceCount==0 && deviceList!=NULL) {
			int count = iterateChpids(updateDeviceList, filterDevices);
			deviceCount = min(count, MAX_CHPID_COUNT);
		}
		pthread_mutex_unlock(&mutex);
		return true;
	}
	return false;
}

int metricRetrCUE( int mid, 
			   MetricReturner mret ) { 
		
	size_t i, count;
	
	if (!refreshDeviceList()) return 0;
		
	for (count=0, i=0; i<deviceCount; ++i) {
		zChannelUtilization cu;
		zChannelMeasurementCharacteristics cmc;
		char* device = deviceList+i*(MAX_PATH_LENGTH+1);
		void* success = readCue(device, &cu);
		if (success!=NULL && cu.cmg==2) {
			success = readCmc(device, &cmc);
			if (success!=NULL && cmc.cmg==2) {
				char* cue_values = stringifyChannelUtilization(&cu);
				char* cmc_values = stringifyChannelMeasurementCharacteristics(&cmc);
				sendMetric(mid, mret, device, cue_values, cmc_values);
				free(cue_values);
				free(cmc_values);
				++count;
			}
		}
	}
	return count;
}

/* ---------------------------------------------------------------------------*/
/*                          end of metricProcessor.c                          */
/* ---------------------------------------------------------------------------*/

