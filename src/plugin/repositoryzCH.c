/*
 * $Id: repositoryzCH.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * Repository Plugin of the IBM System z channel metrics
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <commutil.h>
#include "channelutil.h"

#ifdef DEBUG
#define DBGONLY(X) X
#else
#define DBGONLY(X)
#endif

/* ---------------------------------------------------------------------------*/

/* metric definition block */
static MetricCalculationDefinition metricCalcDef[8];

/* metric calculator callbacks */
static MetricCalculator  metricCalcCUE;
static MetricCalculator  metricCalcPartitionUtilizationPercentage;
static MetricCalculator  metricCalcTotalUtilizationPercentage;
static MetricCalculator  metricCalcBusUtilizationPercentage;
static MetricCalculator  metricCalcPartitionReadThroughput;
static MetricCalculator  metricCalcPartitionWriteThroughput;
static MetricCalculator  metricCalcTotalReadThroughput;
static MetricCalculator  metricCalcTotalWriteThroughput;

/* unit definitions */
static char * muNa = "n/a";
static char * muPercent = "Percent";
static char * muBytesPerSecond ="Bytes/s";
static const float tick = 0.000128; // 128 microseconds
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
  metricCalcDef[0].mcName="_CUE_CMC";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcCUE;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="PartitionUtilizationPercentage";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[1].mcDataType=MD_FLOAT32;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcPartitionUtilizationPercentage;
  metricCalcDef[1].mcUnits=muPercent;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="TotalUtilizationPercentage";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[2].mcDataType=MD_FLOAT32;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcTotalUtilizationPercentage;
  metricCalcDef[2].mcUnits=muPercent;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="BusUtilizationPercentage";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[3].mcDataType=MD_FLOAT32;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcBusUtilizationPercentage;
  metricCalcDef[3].mcUnits=muPercent;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="PartitionReadThroughput";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[4].mcCalc=metricCalcPartitionReadThroughput;
  metricCalcDef[4].mcUnits=muBytesPerSecond;
  
  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="PartitionWriteThroughput";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[5].mcDataType=MD_UINT64;
  metricCalcDef[5].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[5].mcCalc=metricCalcPartitionWriteThroughput;
  metricCalcDef[5].mcUnits=muBytesPerSecond;
  
  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="TotalReadThroughput";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_UINT64;
  metricCalcDef[6].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[6].mcCalc=metricCalcTotalReadThroughput;
  metricCalcDef[6].mcUnits=muBytesPerSecond;

  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="TotalWriteThroughput";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_SUMMABLE;
  metricCalcDef[7].mcDataType=MD_UINT64;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcTotalWriteThroughput;
  metricCalcDef[7].mcUnits=muBytesPerSecond;
  
  *mcnum=8;
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* Raw data                                                                   */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCUE( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {

  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CUE\n", __FILE__,__LINE__);)

  if ( mv ) {
    strncpy(v, mv->mvData, vlen-1);
    ((char*)v)[vlen-1] = '\0';
    return strlen(v);
  }
  return -1;
}

void parseMetricBlock(char* block, zChannelUtilization* cu, zChannelMeasurementCharacteristics* cmc) {
	if (block==NULL) return;
	char* sep = strstr(block, "!");
	if (sep==NULL) return;
	destringifyChannelUtilization(block, cu);
	destringifyChannelMeasurementCharacteristics(++sep, cmc);
}

long long max_ll(long long lhs, long long rhs) {
	return (lhs>rhs) ? lhs : rhs;
}

static unsigned int wrapsafe_24delta(unsigned int new, unsigned int old)
{
  if (new < old) {
    return 0x01000000 - (old - new);
  } else {
    return new - old;
  }
}

/* -------------------------------------------------------------------------- */
/* Calculated metrics                                                         */
/* -------------------------------------------------------------------------- */

size_t metricCalcPartitionUtilizationPercentage( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate PartitionUtilizationPercentage\n", __FILE__,__LINE__);)

	float result = 0;
	
	if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
	  	
	  	if (!(cu1.cuiv & 0x10 && cu2.cuiv & 0x10)) return -1;
	  	
	    int work_units = cu1.block2.lpar_channel_work_units - cu2.block2.lpar_channel_work_units;
	    float interval = wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick;
	    unsigned int max_work_units = cmc.block2.maximum_channel_work_units;
	    float denominator = interval * max_work_units;
	    if (denominator != 0.0) {
		 	result = (work_units * 100.0) / (denominator);
	    }
	    memcpy(v,&result,sizeof(float));
	    return sizeof(float);    
	}
  
	return -1;
}

size_t metricCalcTotalUtilizationPercentage( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate TotalUtilizationPercentage\n", __FILE__,__LINE__);)

	float result = 0;
	
	if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
	    int work_units = cu1.block2.cpc_channel_work_units - cu2.block2.cpc_channel_work_units;
	    float interval = wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick;
	    unsigned int max_work_units = cmc.block2.maximum_channel_work_units;
	    float denominator = interval * max_work_units;
	    if (denominator != 0.0) {
		 	result = (work_units * 100.0) / (denominator);
	    }
	    memcpy(v,&result,sizeof(float));
	    return sizeof(float);    
	}
  
	return -1;
}

size_t metricCalcBusUtilizationPercentage( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate BusUtilizationPercentage\n", __FILE__,__LINE__);)

	float result = 0;
	
	if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
	    int bus_cycles = cu1.block2.cpc_bus_cycles - cu2.block2.cpc_bus_cycles;
	    float interval = wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick;
	    unsigned int max_bus_cycles = cmc.block2.maximum_bus_cycles;
	    float denominator = interval * max_bus_cycles;
	    if (denominator != 0.0) {
		 	result = (bus_cycles * 100.0) / (denominator);
	    }
	    memcpy(v,&result,sizeof(float));
	    return sizeof(float);    
	}
  
	return -1;
}

size_t metricCalcPartitionReadThroughput( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate PartitionReadThroughput\n", __FILE__,__LINE__);)

	unsigned long long result = 0;
	
	if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
		time_t interval;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
		interval = ceil(wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick);

	  	if (!(cu1.cuiv & 0x01 && cu2.cuiv & 0x01)) return -1;
	  	
	  	long long units =  cu1.block2.lpar_data_units_read - cu2.block2.lpar_data_units_read;
 	    if (interval > 0 ) {
	      result = max_ll(units, 0LL) * cmc.block2.data_unit_size / interval;
	    } else {
	      result = 0LL;
	    }
	    memcpy(v,&result,sizeof(unsigned long long));
	    return sizeof(unsigned long long);    
	}
  
	return -1;
}

size_t metricCalcPartitionWriteThroughput( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate PartitionWriteThroughput\n", __FILE__,__LINE__);)

	unsigned long long result = 0;
	
	if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
		time_t interval;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
		interval = ceil(wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick);
	  	
	  	if (!(cu1.cuiv & 0x04 && cu2.cuiv & 0x04)) return -1;
	  	
	  	long long units =  cu1.block2.lpar_data_units_written - cu2.block2.lpar_data_units_written;
 	    if (interval > 0 ) {
	      result = max_ll(units, 0LL) * cmc.block2.data_unit_size / interval;
	    } else {
	      result = 0LL;
	    }
	    memcpy(v,&result,sizeof(unsigned long long));
	    return sizeof(unsigned long long);    
	}
  
	return -1;
}

size_t metricCalcTotalReadThroughput( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate TotalReadThroughput\n", __FILE__,__LINE__);)

	unsigned long long result = 0;
	
	if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
		time_t interval;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
		interval = ceil(wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick);

	  	long long units =  cu1.block2.cpc_data_units_read - cu2.block2.cpc_data_units_read;
 	    if (interval > 0 ) {
	      result = max_ll(units, 0LL) * cmc.block2.data_unit_size / interval;
	    } else {
	      result = 0LL;
	    }
	    memcpy(v,&result,sizeof(unsigned long long));
	    return sizeof(unsigned long long);    
	}
  
	return -1;
}

size_t metricCalcTotalWriteThroughput( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) { 

	DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate TotalWriteThroughput\n", __FILE__,__LINE__);)

	unsigned long long result = 0;
	
	if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
	  	zChannelUtilization cu1;
	  	zChannelUtilization cu2;
	  	zChannelMeasurementCharacteristics cmc;
		time_t interval;
	  	parseMetricBlock(mv[0].mvData, &cu1, &cmc);
	  	parseMetricBlock(mv[mnum-1].mvData, &cu2, NULL);
		interval = ceil(wrapsafe_24delta(cu1.timestamp, cu2.timestamp) * tick);

	  	long long units =  cu1.block2.cpc_data_units_written - cu2.block2.cpc_data_units_written;
 	    if (interval > 0 ) {
	      result = max_ll(units, 0LL) * cmc.block2.data_unit_size / interval;
	    } else {
	      result = 0LL;
	    }
	    memcpy(v,&result,sizeof(unsigned long long));
	    return sizeof(unsigned long long);    
	}
  
	return -1;
}

/* ---------------------------------------------------------------------------*/
/*                         end of repositoryProcessor.c                       */
/* ---------------------------------------------------------------------------*/
