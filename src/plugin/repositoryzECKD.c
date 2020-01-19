/*
 * $Id: repositoryzECKD.c,v 1.3 2012/07/31 23:26:46 tyreld Exp $
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
 * Repository Plugin of the IBM System z ECKD volume metrics
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commutil.h>

#ifdef DEBUG
#define DBGONLY(X) X
#else
#define DBGONLY(X)
#endif

/* ---------------------------------------------------------------------------*/

/* metric definition block */
static MetricCalculationDefinition metricCalcDef[10];

/* metric calculator callbacks */
static MetricCalculator  metricCalcECKD;
static MetricCalculator  metricCalcResponseTime;
static MetricCalculator  metricCalcICmdResponseTime;
static MetricCalculator  metricCalcControlUnitQueueTime;
static MetricCalculator  metricCalcPendingTime;
static MetricCalculator  metricCalcConnectTime;
static MetricCalculator  metricCalcDisconnectTime;
static MetricCalculator  metricCalcBusyTimePercentage;
static MetricCalculator  metricCalcRequestRate;
static MetricCalculator  metricCalcIoIntensity;

/* unit definitions */
static char * muNa = "n/a";
static char * muMilliSeconds = "Milliseconds";
static char * muPercent = "Percent";
static char * muPerSecond = "1/s";
static char * muMsPerSecond = "ms/s";

/* ---------------------------------------------------------------------------*/

int _DefinedRepositoryMetrics( MetricRegisterId *mr,
			       const char *pluginname,
			       size_t *mcnum,
			       MetricCalculationDefinition **mc ) {
  DBGONLY(fprintf(stderr,"--- %s(%i) : Retrieving metric calculation definitions\n", __FILE__,__LINE__);)

  if (mr==NULL||mcnum==NULL||mc==NULL) {
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
    return -1;
  }

  metricCalcDef[0].mcVersion=MD_VERSION;
  metricCalcDef[0].mcName="_ECKD";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcECKD;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="ResponseTime";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[1].mcDataType=MD_FLOAT32;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcResponseTime;
  metricCalcDef[1].mcUnits=muMilliSeconds;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="InitialCommandResponseTime";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[2].mcDataType=MD_FLOAT32;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcICmdResponseTime;
  metricCalcDef[2].mcUnits=muMilliSeconds;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="ControlUnitQueueTime";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[3].mcDataType=MD_FLOAT32;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcControlUnitQueueTime;
  metricCalcDef[3].mcUnits=muMilliSeconds;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="PendingTime";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[4].mcDataType=MD_FLOAT32;
  metricCalcDef[4].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[4].mcCalc=metricCalcPendingTime;
  metricCalcDef[4].mcUnits=muMilliSeconds;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="ConnectTime";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[5].mcDataType=MD_FLOAT32;
  metricCalcDef[5].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[5].mcCalc=metricCalcConnectTime;
  metricCalcDef[5].mcUnits=muMilliSeconds;
  
  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="DisconnectTime";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_FLOAT32;
  metricCalcDef[6].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[6].mcCalc=metricCalcDisconnectTime;
  metricCalcDef[6].mcUnits=muMilliSeconds;
  
  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="AverageDeviceUtilization";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_FLOAT32;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcBusyTimePercentage;
  metricCalcDef[7].mcUnits=muPercent;

  metricCalcDef[8].mcVersion=MD_VERSION;
  metricCalcDef[8].mcName="RequestRate";
  metricCalcDef[8].mcId=mr(pluginname,metricCalcDef[8].mcName);
  metricCalcDef[8].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[8].mcChangeType=MD_GAUGE;
  metricCalcDef[8].mcIsContinuous=MD_TRUE;
  metricCalcDef[8].mcCalculable=MD_SUMMABLE;
  metricCalcDef[8].mcDataType=MD_FLOAT32;
  metricCalcDef[8].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[8].mcCalc=metricCalcRequestRate;
  metricCalcDef[8].mcUnits=muPerSecond;
  
  metricCalcDef[9].mcVersion=MD_VERSION;
  metricCalcDef[9].mcName="IOIntensity";
  metricCalcDef[9].mcId=mr(pluginname,metricCalcDef[9].mcName);
  metricCalcDef[9].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[9].mcChangeType=MD_GAUGE;
  metricCalcDef[9].mcIsContinuous=MD_TRUE;
  metricCalcDef[9].mcCalculable=MD_SUMMABLE;
  metricCalcDef[9].mcDataType=MD_FLOAT32;
  metricCalcDef[9].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[9].mcCalc=metricCalcIoIntensity;
  metricCalcDef[9].mcUnits=muMsPerSecond;
   
  *mcnum=10;
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* Raw data                                                                   */
/* ---------------------------------------------------------------------------*/

static size_t metricCalcECKD( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {

  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate TotalCPUTimePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=strlen(mv->mvData))) {
    strcpy(v, mv->mvData);
    return strlen(v);
  }
  return -1;
}

/* -------------------------------------------------------------------------- */
/* Calculated metrics                                                         */
/* -------------------------------------------------------------------------- */

typedef struct {
	unsigned long long avg_control_unit_queuing_time;
	unsigned long long avg_device_active_only_time;
	unsigned long long avg_device_busy_time;
	unsigned long long avg_device_connect_time;
	unsigned long long avg_device_disconnect_time;
	unsigned long long avg_function_pending_time;
	unsigned long long avg_initial_command_response_time;
	unsigned long long avg_sample_interval;
	float avg_utilization;
	unsigned long long ssch_rsch_count;
	unsigned long long sample_count;
} ECKDMetric;

ECKDMetric parseMetricBlock(char * data) {
	ECKDMetric result;
	sscanf(data, "%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%f%*c:%llu:%llu:", 
					&result.avg_control_unit_queuing_time,
					&result.avg_device_active_only_time,
					&result.avg_device_busy_time,
					&result.avg_device_connect_time,
					&result.avg_device_disconnect_time,
					&result.avg_function_pending_time,
					&result.avg_initial_command_response_time,
					&result.avg_sample_interval,
					&result.avg_utilization,
					&result.ssch_rsch_count,
					&result.sample_count);
	return result;
}	

static unsigned long long getConnectTime(ECKDMetric currentSample, ECKDMetric previousSample) {
  if (currentSample.avg_device_connect_time*currentSample.sample_count >
      previousSample.avg_device_connect_time*previousSample.sample_count ) {
    return currentSample.avg_device_connect_time*currentSample.sample_count 
      - previousSample.avg_device_connect_time*previousSample.sample_count;
  } else {
    return 0ULL;
  }
}

static unsigned long long getDisconnectTime(ECKDMetric currentSample, ECKDMetric previousSample) {
  if (currentSample.avg_device_disconnect_time*currentSample.sample_count >
      previousSample.avg_device_disconnect_time*previousSample.sample_count ) {
    return currentSample.avg_device_disconnect_time*currentSample.sample_count 
	    		- previousSample.avg_device_disconnect_time*previousSample.sample_count;
  } else {
    return 0ULL;
  }
}

static unsigned long long getICmdResponseTime(ECKDMetric currentSample, ECKDMetric previousSample) {
  if (currentSample.avg_initial_command_response_time*currentSample.sample_count >
      previousSample.avg_initial_command_response_time*previousSample.sample_count) {   
	return currentSample.avg_initial_command_response_time*currentSample.sample_count 
			- previousSample.avg_initial_command_response_time*previousSample.sample_count;
  } else {
    return 0ULL;
  }
}

static unsigned long long getControlUnitQueueTime(ECKDMetric currentSample, ECKDMetric previousSample) {
  if (currentSample.avg_control_unit_queuing_time*currentSample.sample_count >
      previousSample.avg_control_unit_queuing_time*previousSample.sample_count) {
    return currentSample.avg_control_unit_queuing_time*currentSample.sample_count 
      - previousSample.avg_control_unit_queuing_time*previousSample.sample_count;
  } else {
    return 0ULL;
  }
}

static unsigned long long getPendingTime(ECKDMetric currentSample, ECKDMetric previousSample) {
	return getICmdResponseTime(currentSample,previousSample)
			+ getControlUnitQueueTime(currentSample, previousSample);
}

static unsigned long long getResponseTime(ECKDMetric currentSample, ECKDMetric previousSample) {
	return getPendingTime(currentSample, previousSample) 
			+ getConnectTime(currentSample, previousSample) 
			+ getDisconnectTime(currentSample, previousSample);
}

#ifdef USE_SAMPLED_INTERVAL 					
static unsigned long long getInterval(ECKDMetric currentSample, ECKDMetric previousSample) {
    return currentSample.avg_sample_interval*currentSample.sample_count 
			- previousSample.avg_sample_interval*previousSample.sample_count;
}
#else    							
static unsigned long long getElapsedInterval(MetricValue* mv, size_t mnum) {
  if (mv && mnum > 1) {
    return (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp)*1000000000ULL;
  } else {
    return 0ULL;
  }
}    		

#endif

static float getBusyTimeFraction(ECKDMetric currentSample, ECKDMetric previousSample, unsigned long long interval) {

	unsigned long long busyTime = getConnectTime(currentSample, previousSample)
									+ getDisconnectTime(currentSample, previousSample);	
    return (interval != 0LL) ? ((float) busyTime / (float) interval) : 0.0;
}

static unsigned long long getRequestCount(ECKDMetric currentSample, ECKDMetric previousSample) {
	return currentSample.ssch_rsch_count - previousSample.ssch_rsch_count;
}


static size_t metricCalcConnectTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate ConnectTime\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getConnectTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}


static size_t metricCalcResponseTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Response Time\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getResponseTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcICmdResponseTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Initial Command Response Time\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getICmdResponseTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcControlUnitQueueTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Control Unit Queue Time\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getControlUnitQueueTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcPendingTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Pending Time\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getPendingTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcDisconnectTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Disconnect Time\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
    unsigned long long requestCount = getRequestCount(sample1, sample2);
    result = (requestCount != 0LL) ? (getDisconnectTime(sample1, sample2) / ( 1000000.0 * (float) requestCount )) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcBusyTimePercentage( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Busy Time Percentage\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);

#ifdef USE_SAMPLED_INTERVAL 					
    result = getBusyTimeFraction(sample1, sample2, getInterval(sample1,sample2)) * 100.0;
#else
    result = getBusyTimeFraction(sample1, sample2, getElapsedInterval(mv,mnum)) * 100.0;
#endif
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcRequestRate( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate Request Rate\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	unsigned long long interval;;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);
#ifdef USE_SAMPLED_INTERVAL 					
    interval = getInterval(sample1,sample2);
#else
    interval = getElapsedInterval(mv,mnum);
#endif
    result = (interval != 0LL) ? ((float) getRequestCount(sample1, sample2)  * 1000000000.0 / (float) interval) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

static size_t metricCalcIoIntensity( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate IO Intensity\n", __FILE__,__LINE__);)

	ECKDMetric sample1;
	ECKDMetric sample2;
	float result;
	unsigned long long interval;
	
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    sample1 = parseMetricBlock(mv[0].mvData);
    sample2 = parseMetricBlock(mv[mnum-1].mvData);

#ifdef USE_SAMPLED_INTERVAL 					
    interval = getInterval(sample1,sample2);
#else
    interval = getElapsedInterval(mv,mnum);
#endif
    result = (interval != 0LL) ? (1000.0 * (float) (getRequestCount(sample1, sample2) * getResponseTime(sample1, sample2)) / (float) interval) : 0.0;
    memcpy(v,&result,sizeof(float));
    return sizeof(float);    
  }
  
  return -1;
}

/* ---------------------------------------------------------------------------*/
/*                         end of repositoryProcessor.c                       */
/* ---------------------------------------------------------------------------*/
