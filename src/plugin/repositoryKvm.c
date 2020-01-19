/*
 * $Id: repositoryKvm.c,v 1.11 2012/07/31 23:26:45 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2009, 2011
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Tyrel Datwyler	<tyreld@us.ibm.com>
 * Contributors: 
 *
 * Description:
 * Repository Plugin of the following KVM specific metrics :
 * 
 *    TotalCPUTime
 *    ActiveVirtualProcessors
 *    ExternalViewTotalCPUPercentage
 *    PhysicalMemoryAllocatedToVirtualSystem
 *    HostFreePhysicalMemory
 *    PhysicalMemoryAllocatedToVirtualSystemPercentage
 *    HostMemoryPercentage
 *    TenMinuteTotalCPUTime
 *	  TenMinuteExternalViewTotalCPUTimePercentage
 *	  VirtualSystemState
 * 
 * plus the following metrics which are only intended for internal usage:
 *    _Internal_CPUTime
 *    _Internal_TotalCPUTime
 *    _Internal_Memory
 *    _Internal10m_CPUTime
 *    _Internal10m_TotalCPUTime
 *
*/

/* ---------------------------------------------------------------------------*/

// #define DEBUG

#include <mplugin.h>
#include <commutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[21];

// metric _Internal_CPUTime
static MetricCalculator metricCalcCPUTime;

// metric _Internal_TotalCPUTime
static MetricCalculator metricCalcIntTotalCPUTime;

// metric TotalCPUTime
static MetricCalculator metricCalcTotalCPUTime;

// metric ExternalViewTotalCPUPercentage
static MetricCalculator metricCalcExtTotalCPUTimePerc;

// metric ActiveVirtualProcessors 
static MetricCalculator metricCalcActiveVirtualProcessors;

// metric PhysicalMemoryAllocatedToVirtualSystem
static MetricCalculator metricCalcPhysicalMemoryAllocatedToVirtualSystem;

// metric HostFreePhysicalMemory
static MetricCalculator metricCalcHostFreePhysicalMemory;

// internal metric - only implemented for internal usage
static MetricCalculator metricCalcInternal;

// metric PhysicalMemoryAllocatedToVirtualSystemPercentage
static MetricCalculator metricCalcPhysicalMemoryAllocatedToVirtualSystemPercentage;

// metric HostMemoryPercentage
static MetricCalculator metricCalcHostMemoryPercentage;

// metric VirtualSystemState
static MetricCalculator metricCalcVirtualSystemState;

// metric SchedulerStats
static MetricCalculator metricCalcSchedulerStats;

// metric SumExecRuntime
// static MetricCalculator metricCalcSumExecRuntime

// metric WaitSum
// static MetricCalculator metricCalcWaitSum

// metric VirtualBlockIOStats
static MetricCalculator metricCalcVirtualBlockIOStats;
static MetricCalculator metricCalcBlockRead;
static MetricCalculator metricCalcBlockWrite;
static MetricCalculator metricCalcBlockCapacity;


/* unit definitions */
static char *muKiloBytes = "Kilobytes";
static char *muBytes = "Bytes";
static char *muPercent = "Percent";
static char *muMicroSeconds = "MicroSeconds";
static char *muMilliSeconds = "MilliSeconds";
static char *muSeconds = "Seconds";
static char *muNA = "N/A";

/* ---------------------------------------------------------------------------*/

enum indices { BLK_READ, BLK_WRITE, BLK_CAP };
static unsigned long long kvm_getData(char * data, char index);


/* ---------------------------------------------------------------------------*/

int _DefinedRepositoryMetrics(MetricRegisterId * mr,
			      const char *pluginname,
			      size_t * mcnum,
			      MetricCalculationDefinition ** mc)
{
#ifdef DEBUG
    fprintf(stderr, "Retrieving metric calculation definitions\n");
#endif

    if (mr == NULL || mcnum == NULL || mc == NULL) {
#ifdef DEBUG
	fprintf(stderr, "invalid parameter list\n");
#endif
	return -1;
    }

    // ----- Xen_ComputerSystem CPU metrics -----

    metricCalcDef[0].mcVersion = MD_VERSION;
    metricCalcDef[0].mcName = "_Internal_CPUTime";
    metricCalcDef[0].mcId = mr(pluginname, metricCalcDef[0].mcName);
    metricCalcDef[0].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[0].mcChangeType = MD_COUNTER;
    metricCalcDef[0].mcIsContinuous = MD_TRUE;
    metricCalcDef[0].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[0].mcDataType = MD_FLOAT32;
    metricCalcDef[0].mcCalc = metricCalcCPUTime;
    metricCalcDef[0].mcUnits = muSeconds;

    metricCalcDef[1].mcVersion = MD_VERSION;
    metricCalcDef[1].mcName = "_Internal_TotalCPUTime";
    metricCalcDef[1].mcId = mr(pluginname, metricCalcDef[1].mcName);
    metricCalcDef[1].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[1].mcChangeType = MD_COUNTER;
    metricCalcDef[1].mcIsContinuous = MD_TRUE;
    metricCalcDef[1].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[1].mcDataType = MD_UINT64;
    metricCalcDef[1].mcCalc = metricCalcIntTotalCPUTime;
    metricCalcDef[1].mcUnits = muMilliSeconds;

    metricCalcDef[2].mcVersion = MD_VERSION;
    metricCalcDef[2].mcName = "TotalCPUTime";
    metricCalcDef[2].mcId = mr(pluginname, metricCalcDef[2].mcName);
    metricCalcDef[2].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL;
    metricCalcDef[2].mcChangeType = MD_GAUGE;
    metricCalcDef[2].mcIsContinuous = MD_TRUE;
    metricCalcDef[2].mcCalculable = MD_SUMMABLE;
    metricCalcDef[2].mcDataType = MD_UINT64;
    metricCalcDef[2].mcAliasId = metricCalcDef[1].mcId;
    metricCalcDef[2].mcCalc = metricCalcTotalCPUTime;
    metricCalcDef[2].mcUnits = muMilliSeconds;

    metricCalcDef[3].mcVersion = MD_VERSION;
    metricCalcDef[3].mcName = "_Internal10m_TotalCPUTime";
    metricCalcDef[3].mcId = mr(pluginname, metricCalcDef[3].mcName);
    metricCalcDef[3].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[3].mcChangeType = MD_COUNTER;
    metricCalcDef[3].mcIsContinuous = MD_TRUE;
    metricCalcDef[3].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[3].mcDataType = MD_UINT64;
    metricCalcDef[3].mcCalc = metricCalcIntTotalCPUTime;
    metricCalcDef[3].mcUnits = muMilliSeconds;

    metricCalcDef[4].mcVersion = MD_VERSION;
    metricCalcDef[4].mcName = "ActiveVirtualProcessors";
    metricCalcDef[4].mcId = mr(pluginname, metricCalcDef[4].mcName);
    metricCalcDef[4].mcMetricType =
	MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[4].mcChangeType = MD_GAUGE;
    metricCalcDef[4].mcIsContinuous = MD_TRUE;
    metricCalcDef[4].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[4].mcDataType = MD_FLOAT32;
    metricCalcDef[4].mcCalc = metricCalcActiveVirtualProcessors;
    metricCalcDef[4].mcUnits = muNA;

    metricCalcDef[5].mcVersion = MD_VERSION;
    metricCalcDef[5].mcName = "ExternalViewTotalCPUTimePercentage";
    metricCalcDef[5].mcId = mr(pluginname, metricCalcDef[5].mcName);
    metricCalcDef[5].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL|MD_ORGSBLIM;
    metricCalcDef[5].mcChangeType = MD_GAUGE;
    metricCalcDef[5].mcIsContinuous = MD_TRUE;
    metricCalcDef[5].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[5].mcDataType = MD_FLOAT32;
    metricCalcDef[5].mcAliasId = metricCalcDef[0].mcId;
    metricCalcDef[5].mcCalc = metricCalcExtTotalCPUTimePerc;
    metricCalcDef[5].mcUnits = muPercent;

    metricCalcDef[6].mcVersion = MD_VERSION;
    metricCalcDef[6].mcName = "_Internal10m_CPUTime";
    metricCalcDef[6].mcId = mr(pluginname, metricCalcDef[6].mcName);
    metricCalcDef[6].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[6].mcChangeType = MD_COUNTER;
    metricCalcDef[6].mcIsContinuous = MD_TRUE;
    metricCalcDef[6].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[6].mcDataType = MD_FLOAT32;
    metricCalcDef[6].mcCalc = metricCalcCPUTime;
    metricCalcDef[6].mcUnits = muSeconds;

    // ----- Xen_ComputerSystem Memory metrics -----
   
    metricCalcDef[7].mcVersion = MD_VERSION;
    metricCalcDef[7].mcName = "_Internal_Memory";
    metricCalcDef[7].mcId = mr(pluginname, metricCalcDef[7].mcName);
    metricCalcDef[7].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[7].mcIsContinuous = MD_FALSE;
    metricCalcDef[7].mcCalculable = MD_NONCALCULABLE;
    metricCalcDef[7].mcDataType = MD_STRING;
    metricCalcDef[7].mcCalc = metricCalcInternal;
    metricCalcDef[7].mcUnits = muNA;

    metricCalcDef[8].mcVersion = MD_VERSION;
    metricCalcDef[8].mcName = "PhysicalMemoryAllocatedToVirtualSystem";
    metricCalcDef[8].mcId = mr(pluginname, metricCalcDef[8].mcName);
    metricCalcDef[8].mcMetricType = MD_PERIODIC | MD_CALCULATED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[8].mcChangeType = MD_GAUGE;
    metricCalcDef[8].mcIsContinuous = MD_TRUE;
    metricCalcDef[8].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[8].mcDataType = MD_UINT64;
    metricCalcDef[8].mcCalc = metricCalcPhysicalMemoryAllocatedToVirtualSystem;
    metricCalcDef[8].mcAliasId = metricCalcDef[7].mcId;
    metricCalcDef[8].mcUnits = muKiloBytes;

    metricCalcDef[9].mcVersion = MD_VERSION;
    metricCalcDef[9].mcName = "HostFreePhysicalMemory";
    metricCalcDef[9].mcId = mr(pluginname, metricCalcDef[9].mcName);
    metricCalcDef[9].mcMetricType = MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[9].mcChangeType = MD_GAUGE;
    metricCalcDef[9].mcIsContinuous = MD_TRUE;
    metricCalcDef[9].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[9].mcDataType = MD_UINT64;
    metricCalcDef[9].mcCalc = metricCalcHostFreePhysicalMemory;
    metricCalcDef[9].mcUnits = muKiloBytes;

    metricCalcDef[10].mcVersion = MD_VERSION;
    metricCalcDef[10].mcName = "PhysicalMemoryAllocatedToVirtualSystemPercentage";
    metricCalcDef[10].mcId = mr(pluginname, metricCalcDef[10].mcName);
    metricCalcDef[10].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[10].mcChangeType = MD_GAUGE;
    metricCalcDef[10].mcIsContinuous = MD_TRUE;
    metricCalcDef[10].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[10].mcDataType = MD_FLOAT32;
    metricCalcDef[10].mcCalc = metricCalcPhysicalMemoryAllocatedToVirtualSystemPercentage;
    metricCalcDef[10].mcAliasId = metricCalcDef[7].mcId;
    metricCalcDef[10].mcUnits = muPercent;

    metricCalcDef[11].mcVersion = MD_VERSION;
    metricCalcDef[11].mcName = "HostMemoryPercentage";
    metricCalcDef[11].mcId = mr(pluginname, metricCalcDef[11].mcName);
    metricCalcDef[11].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[11].mcChangeType = MD_GAUGE;
    metricCalcDef[11].mcIsContinuous = MD_TRUE;
    metricCalcDef[11].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[11].mcDataType = MD_FLOAT32;
    metricCalcDef[11].mcCalc = metricCalcHostMemoryPercentage;
    metricCalcDef[11].mcAliasId = metricCalcDef[7].mcId;
    metricCalcDef[11].mcUnits = muPercent;

    metricCalcDef[12].mcVersion = MD_VERSION;
    metricCalcDef[12].mcName = "TenMinuteTotalCPUTime";
    metricCalcDef[12].mcId = mr(pluginname, metricCalcDef[12].mcName);
    metricCalcDef[12].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL|MD_ORGSBLIM;
    metricCalcDef[12].mcChangeType = MD_GAUGE;
    metricCalcDef[12].mcIsContinuous = MD_TRUE;
    metricCalcDef[12].mcCalculable = MD_SUMMABLE;
    metricCalcDef[12].mcDataType = MD_UINT64;
    metricCalcDef[12].mcAliasId = metricCalcDef[3].mcId;
    metricCalcDef[12].mcCalc = metricCalcTotalCPUTime;
    metricCalcDef[12].mcUnits = muMilliSeconds;

    metricCalcDef[13].mcVersion = MD_VERSION;
    metricCalcDef[13].mcName =
	"TenMinuteExternalViewTotalCPUTimePercentage";
    metricCalcDef[13].mcId = mr(pluginname, metricCalcDef[13].mcName);
    metricCalcDef[13].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL|MD_ORGSBLIM;
    metricCalcDef[13].mcChangeType = MD_GAUGE;
    metricCalcDef[13].mcIsContinuous = MD_TRUE;
    metricCalcDef[13].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[13].mcDataType = MD_FLOAT32;
    metricCalcDef[13].mcAliasId = metricCalcDef[6].mcId;
    metricCalcDef[13].mcCalc = metricCalcExtTotalCPUTimePerc;
    metricCalcDef[13].mcUnits = muPercent;

    metricCalcDef[14].mcVersion = MD_VERSION;
    metricCalcDef[14].mcName = "VirtualSystemState";
    metricCalcDef[14].mcId = mr(pluginname, metricCalcDef[14].mcName);
    metricCalcDef[14].mcMetricType =
	MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[14].mcIsContinuous = MD_FALSE;
    metricCalcDef[14].mcCalculable = MD_NONCALCULABLE;
    metricCalcDef[14].mcDataType = MD_UINT32;
    metricCalcDef[14].mcCalc = metricCalcVirtualSystemState;
    metricCalcDef[14].mcUnits = muNA;

    metricCalcDef[15].mcVersion = MD_VERSION;
    metricCalcDef[15].mcName = "CPUUsedTimeCounter";
    metricCalcDef[15].mcId = mr(pluginname, metricCalcDef[15].mcName);
    metricCalcDef[15].mcMetricType =
	MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[15].mcChangeType = MD_COUNTER;
    metricCalcDef[15].mcIsContinuous = MD_TRUE;
    metricCalcDef[15].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[15].mcDataType = MD_UINT64;
    metricCalcDef[15].mcCalc = metricCalcSchedulerStats;
    metricCalcDef[15].mcUnits = muMicroSeconds;
    
    metricCalcDef[16].mcVersion = MD_VERSION;
    metricCalcDef[16].mcName = "CPUReadyTimeCounter";
    metricCalcDef[16].mcId = mr(pluginname, metricCalcDef[16].mcName);
    metricCalcDef[16].mcMetricType =
	MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[16].mcChangeType = MD_COUNTER;
    metricCalcDef[16].mcIsContinuous = MD_TRUE;
    metricCalcDef[16].mcCalculable = MD_NONSUMMABLE;
    metricCalcDef[16].mcDataType = MD_UINT64;
    metricCalcDef[16].mcCalc = metricCalcSchedulerStats;
    metricCalcDef[16].mcUnits = muMicroSeconds;
    
    metricCalcDef[17].mcVersion = MD_VERSION;
    metricCalcDef[17].mcName = "_VirtualBlockIOStats";
    metricCalcDef[17].mcId = mr(pluginname, metricCalcDef[17].mcName);
    metricCalcDef[17].mcMetricType =
	MD_PERIODIC | MD_RETRIEVED | MD_POINT|MD_ORGSBLIM;
    metricCalcDef[17].mcIsContinuous = MD_FALSE;
    metricCalcDef[17].mcCalculable = MD_NONCALCULABLE;
    metricCalcDef[17].mcDataType = MD_STRING;
    metricCalcDef[17].mcCalc = metricCalcVirtualBlockIOStats;
    metricCalcDef[17].mcUnits = muNA;
    
    metricCalcDef[18].mcVersion = MD_VERSION;
    metricCalcDef[18].mcName = "BlockRead";
    metricCalcDef[18].mcId = mr(pluginname, metricCalcDef[18].mcName);
    metricCalcDef[18].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL|MD_ORGSBLIM;
    metricCalcDef[18].mcChangeType = MD_GAUGE;
    metricCalcDef[18].mcIsContinuous = MD_TRUE;
    metricCalcDef[18].mcCalculable = MD_SUMMABLE;
    metricCalcDef[18].mcDataType = MD_UINT64;
    metricCalcDef[18].mcAliasId=metricCalcDef[17].mcId;
    metricCalcDef[18].mcCalc = metricCalcBlockRead;
    metricCalcDef[18].mcUnits = muKiloBytes;
    
    metricCalcDef[19].mcVersion = MD_VERSION;
    metricCalcDef[19].mcName = "BlockWrite";
    metricCalcDef[19].mcId = mr(pluginname, metricCalcDef[19].mcName);
    metricCalcDef[19].mcMetricType =
	MD_PERIODIC | MD_CALCULATED | MD_INTERVAL|MD_ORGSBLIM;
    metricCalcDef[19].mcChangeType = MD_GAUGE;
    metricCalcDef[19].mcIsContinuous = MD_TRUE;
    metricCalcDef[19].mcCalculable = MD_SUMMABLE;
    metricCalcDef[19].mcDataType = MD_UINT64;
    metricCalcDef[19].mcAliasId=metricCalcDef[17].mcId;
    metricCalcDef[19].mcCalc = metricCalcBlockWrite;
    metricCalcDef[19].mcUnits = muKiloBytes;
    
    metricCalcDef[20].mcVersion=MD_VERSION;
    metricCalcDef[20].mcName="BlockCapacity";
    metricCalcDef[20].mcId=mr(pluginname,metricCalcDef[20].mcName);
    metricCalcDef[20].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT|MD_ORGSBLIM;
    metricCalcDef[20].mcChangeType=MD_GAUGE;
    metricCalcDef[20].mcIsContinuous=MD_TRUE;
    metricCalcDef[20].mcCalculable=MD_NONSUMMABLE;
    metricCalcDef[20].mcDataType=MD_UINT64;
    metricCalcDef[20].mcAliasId=metricCalcDef[17].mcId;
    metricCalcDef[20].mcCalc=metricCalcBlockCapacity;
    metricCalcDef[20].mcUnits=muBytes;

    *mcnum = 21;
    *mc = metricCalcDef;
    return 0;
}



/* ---------------------------------------------------------------------------*/
/* _Internal_CPUTime                                                          */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCPUTime(MetricValue * mv, int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "retrieve internal Xen CPU time (float), mnum: %d\n",
	    mnum);
#endif

    // plain copy
    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1)) {
	memcpy(v, mv->mvData, mv->mvDataLength);
	*(float *) v = ntohf(*(float *) v);
	return mv->mvDataLength;
    }
    return -1;
}


/* ---------------------------------------------------------------------------*/
/* internal TotalCPUTime                                                      */
/* ---------------------------------------------------------------------------*/

size_t metricCalcIntTotalCPUTime(MetricValue * mv,
				 int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "retrieve internal Xen CPU time (uint64), mnum: %d\n",
	    mnum);
#endif

    /* plain copy */
    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1)) {
	memcpy(v, mv->mvData, mv->mvDataLength);
	*(unsigned long long *) v = ntohll(*(unsigned long long *) v);
	return mv->mvDataLength;
    }

    return -1;
}

/* ---------------------------------------------------------------------------*/
/* TotalCPUTime                                                               */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalCPUTime(MetricValue * mv,
			      int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "retrieve Xen CPU time (uint64), mnum: %d\n", mnum);
#endif
    /* plain copy */
    if (mv && (mnum >= 2)) {
	unsigned long long start_time =
	    ntohll(*(unsigned long long *) mv[mnum - 1].mvData);
	unsigned long long end_time =
	    ntohll(*(unsigned long long *) mv[0].mvData);

	unsigned long long cputime = end_time - start_time;
    if (end_time == 0) cputime = 0;

	memcpy(v, &cputime, sizeof(unsigned long long));
	return sizeof(unsigned long long);
    }
    return -1;
}


/* ---------------------------------------------------------------------------*/
/* ExternalViewTotalCPUTimePercentage                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcExtTotalCPUTimePerc(MetricValue * mv,
				     int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "Calculate Xen total CPU time, mnum: %d\n", mnum);
#endif
    if (mv && (vlen >= mv->mvDataLength) && (mnum >= 2)) {
	float start_time = ntohf(*(float *) mv[mnum - 1].mvData);
	float end_time = ntohf(*(float *) mv[0].mvData);
#ifdef DEBUG
	fprintf(stderr, "start_time %f  end_time %f\n", start_time,
		end_time);
#endif

	// interval length in seconds
	unsigned interval_length =
	    mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp;

#ifdef DEBUG
	fprintf(stderr, "interval length %d\n", interval_length);
#endif

	float result = (end_time - start_time) / interval_length * 100;
    if (end_time == 0) result = 0;

	memcpy(v, &result, sizeof(float));
	return sizeof(float);
    }
    return -1;
}


/* ---------------------------------------------------------------------------*/
/* ActiveVirtualProcessors                                                    */
/* ---------------------------------------------------------------------------*/

size_t metricCalcActiveVirtualProcessors(MetricValue * mv,
					 int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "Calculate active virtual processors\n");
#endif

    // it's an eServer interval metric, however on Xen the number of processors
    // cannot currently change dynamically so just one of the values is actually
    // picked
    if (mv && (mnum >= 1)) {
	memcpy(v, mv[0].mvData, mv->mvDataLength);
	return mv->mvDataLength;
    }

    return -1;
}


/* ---------------------------------------------------------------------------*/
/* PhysicalMemoryAllocatedToVirtualSystem                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPhysicalMemoryAllocatedToVirtualSystem(MetricValue * mv,
					int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Calculate partition claimed memory metric\n");
#endif

    if (mv && (mnum >= 1)) {
	char buf[70];
	strncpy((char *) &buf, (char *) mv->mvData, 69);

	unsigned long long claimed_memory = 0;
	unsigned long long total_memory = 0;
	unsigned long long max_memory = 0;
	sscanf(buf, "%lld:%lld:%lld",
	       &claimed_memory, &max_memory, &total_memory);

#ifdef DEBUG
	fprintf(stderr,
		"claimed_memory: %lld  max_memory: %lld  total_memory: %lld\n",
		claimed_memory, max_memory, total_memory);
#endif
	memcpy(v, &claimed_memory, sizeof(unsigned long long));
	return sizeof(unsigned long long);
    }

    return -1;
}


/* ---------------------------------------------------------------------------*/
/* PartitionMaximumMemory                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPartitionMaximumMemory(MetricValue * mv,
					int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Calculate partition maximum memory metric\n");
#endif

    if (mv && (mnum >= 1)) {
	char buf[70];
	strncpy((char *) &buf, (char *) mv->mvData, 69);

	unsigned long long claimed_memory = 0;
	unsigned long long total_memory = 0;
	unsigned long long max_memory = 0;
	sscanf(buf, "%lld:%lld:%lld",
	       &claimed_memory, &max_memory, &total_memory);

#ifdef DEBUG
	fprintf(stderr,
		"claimed_memory: %lld  max_memory: %lld  total_memory: %lld\n",
		claimed_memory, max_memory, total_memory);
#endif

	memcpy(v, (void *) &max_memory, sizeof(unsigned long long));
	return sizeof(unsigned long long);
    }

    return -1;
}



/* ---------------------------------------------------------------------------*/
/* HostFreePhysicalMemory                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcHostFreePhysicalMemory(MetricValue * mv,
					int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Calculate host free physical memory metric\n");
#endif

    /* plain copy */
    if (mv && (mnum >= 1)) {
	memcpy(v, mv->mvData, mv->mvDataLength);
	return mv->mvDataLength;
    }

    return -1;
}


/* ---------------------------------------------------------------------------*/
/* internal metric - not externalized                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcInternal(MetricValue * mv, int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "internal memory\n");
#endif

    /* plain copy */
    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1)) {
	memcpy(v, mv->mvData, mv->mvDataLength);
	return mv->mvDataLength;
    }
    return -1;
}


/* ---------------------------------------------------------------------------*/
/* PhysicalMemoryAllocatedToVirtualSystemPercentage                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPhysicalMemoryAllocatedToVirtualSystemPercentage(MetricValue * mv,
					 int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Calculate claimed memory percentage metric\n");
#endif

    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1)) {
	char buf[70];
	strncpy((char *) buf, (char *) mv->mvData, 69);

	unsigned long long claimed_memory = 0;
	unsigned long long total_memory = 0;
	unsigned long long max_memory = 0;
	sscanf(buf, "%lld:%lld:%lld",
	       &claimed_memory, &max_memory, &total_memory);

#ifdef DEBUG
	fprintf(stderr,
		"claimed_memory: %lld  max_memory: %lld  total_memory: %lld\n",
		claimed_memory, max_memory, total_memory);
#endif
	float result = (float) claimed_memory / (float) max_memory * 100.0;
	memcpy(v, &result, sizeof(float));
	return sizeof(float);
    }

    return -1;
}


/* ---------------------------------------------------------------------------*/
/* HostMemoryPercentage                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcHostMemoryPercentage(MetricValue * mv,
				      int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Calculate host memory percentage metric\n");
#endif

    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1)) {
	char buf[70];
	strncpy((char *) buf, (char *) mv->mvData, 69);

	unsigned long long claimed_memory = 0;
	unsigned long long total_memory = 0;
	unsigned long long max_memory = 0;
	sscanf(buf, "%lld:%lld:%lld",
	       &claimed_memory, &max_memory, &total_memory);

#ifdef DEBUG
	fprintf(stderr,
		"claimed_memory: %lld  max_memory: %lld  total_memory: %lld\n",
		claimed_memory, max_memory, total_memory);
#endif
	float result =
	    (float) claimed_memory / (float) total_memory * 100.0;
	memcpy(v, &result, sizeof(float));
	return sizeof(float);
    }

    return -1;
}

/* ---------------------------------------------------------------------------*/
/* VirtualSystemState                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcVirtualSystemState(MetricValue * mv,
					 int mnum, void *v, size_t vlen)
{

#ifdef DEBUG
    fprintf(stderr, "Calculate Virtual System State\n");
#endif

    if (mv && (mnum >= 1)) {
	memcpy(v, mv[0].mvData, mv->mvDataLength);
	return mv->mvDataLength;
    }

    return -1;
}

/* ---------------------------------------------------------------------------*/
/* Scheduler Statistics                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcSchedulerStats(MetricValue * mv,
                    int mnum, void *v, size_t vlen)
{
#ifdef DEBUG
    fprintf(stderr, "Raw Scheduler Statistics\n");
#endif

    /* plain copy */
    if (mv && (vlen >= mv->mvDataLength) && (mnum == 1) ) {
        memcpy(v, mv->mvData, mv->mvDataLength);
        *(unsigned long long *) v = ntohll(*(unsigned long long *) v);
        return mv->mvDataLength;
    }

    return -1;
}

/* ---------------------------------------------------------------------------*/
/* Virtual Block IO Stats                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcVirtualBlockIOStats( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate VirtualBlockIOStats\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    return mv->mvDataLength;
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* BlockRead                                                                  */
/* ---------------------------------------------------------------------------*/

size_t metricCalcBlockRead( MetricValue *mv,   
				int mnum,
				void *v, 
				size_t vlen ) 
{
  unsigned long long br = 0;
  unsigned long long b1 = 0;
  unsigned long long b2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BlockRead\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    b1 = kvm_getData(mv[0].mvData, BLK_READ);
    if( mnum > 1 ) {
      b2 = kvm_getData(mv[mnum-1].mvData, BLK_READ);
      br = b1-b2;
    }
    else { br = b1; }

    memcpy(v,&br,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* BlockWrite                                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcBlockWrite( MetricValue *mv,   
				int mnum,
				void *v, 
				size_t vlen ) 
{
  unsigned long long br = 0;
  unsigned long long b1 = 0;
  unsigned long long b2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BlockWrite\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    b1 = kvm_getData(mv[0].mvData, BLK_WRITE);
    if( mnum > 1 ) {
      b2 = kvm_getData(mv[mnum-1].mvData, BLK_WRITE);
      br = b1-b2;
    }
    else { br = b1; }

    memcpy(v,&br,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* BlockCapacity                                                                   */
/* ---------------------------------------------------------------------------*/

size_t metricCalcBlockCapacity( MetricValue *mv,   
				   int mnum,
				   void *v, 
				   size_t vlen )
{
  unsigned long long ct = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BlockCapacity\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    ct = kvm_getData(mv[0].mvData, BLK_CAP);

    memcpy(v,&ct,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* tool function on internal raw metrics                                      */
/* ---------------------------------------------------------------------------*/

unsigned long long kvm_getData(char * data, char index)
{
	char * hlp = NULL;
	char * end = NULL;
	char bytes[128];
	unsigned long long val = 0;
	int i = 0;

	hlp = data;
	if ((end = strchr(hlp, ':')) != NULL) {
		for (i = 0; i < index; i++) {
			hlp = ++end;
			if ((end = strchr(hlp, ':')) == NULL) {
				return val;
			}
		}

		memset(bytes, 0, sizeof(bytes));
		strncpy(bytes, hlp, strlen(hlp) - strlen(end));
		val = strtoll(bytes, (char**) NULL, 10);
	}

	return val;
}

/* ---------------------------------------------------------------------------*/
/*                   end of repositoryKvm.c                       			  */
/* ---------------------------------------------------------------------------*/
