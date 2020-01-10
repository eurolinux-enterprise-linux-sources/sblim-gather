/*
 * $Id: repositoryOperatingSystem.c,v 1.19 2010/04/19 23:56:27 tyreld Exp $
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
 * Author:       Heidi Neumann <heidineu@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * Repository Plugin of the following Operating System specific metrics :
 * 
 * NumberOfUsers
 * NumberOfProcesses
 * CPUTime
 * KernelModeTime
 * UserModeTime
 * IOWaitTime
 * TotalCPUTime
 * MemorySize
 * TotalVisibleMemorySize
 * FreePhysicalMemory
 * SizeStoredInPagingFiles
 * FreeSpaceInPagingFiles
 * TotalVirtualMemorySize
 * FreeVirtualMemory
 * PageInCounter
 * PageInRate
 * PageOutCounter
 * PageOutRate
 * LoadCounter
 * LoadAverage
 * InternalViewKernelModePercentage
 * InternalViewUserModePercentage
 * InternalViewIdlePercentage
 * InternalViewTotalCPUPercentage
 * ExternalViewKernelModePercentage
 * ExternalViewUserModePercentage
 * ExternalViewTotalCPUPercentage
 * CPUConsumptionIndex
 * ContextSwitchCounter
 * ContextSwitchRate
 * HardwareInterruptCounter
 * HardwareInterruptRate
 * 
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <commutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <netinet/in.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[34];

/* --- NumberOfUsers --- */
static MetricCalculator metricCalcNumOfUser;

/* --- NumberOfProcesses --- */
static MetricCalculator metricCalcNumOfProc;

/* --- CPUTime is base for :
 * KernelModeTime, UserModeTime, IOWaitTime, TotalCPUTime
 * InternalViewKernelModePercentage, InternalViewUserModePercentage,
 * InternalViewIdlePercentage, InternalViewTotalCPUPercentage,
 * ExternalViewKernelModePercentage, ExternalViewUserModePercentage,
 * ExternalViewTotalCPUPercentage,
 * CPUConsumptionIndex 
 * --- */
static MetricCalculator metricCalcCPUTime;

static MetricCalculator metricCalcKernelTime;
static MetricCalculator metricCalcUserTime;
static MetricCalculator metricCalcIOWaitTime;
static MetricCalculator metricCalcTotalCPUTime;

static MetricCalculator metricCalcInternKernelTimePerc;
static MetricCalculator metricCalcInternUserTimePerc;
static MetricCalculator metricCalcInternIdleTimePerc;
static MetricCalculator metricCalcInternTotalCPUTimePerc;

static MetricCalculator metricCalcExternKernelTimePerc;
static MetricCalculator metricCalcExternUserTimePerc;
static MetricCalculator metricCalcExternTotalCPUTimePerc;

static MetricCalculator metricCalcCPUConsumptionIndex;

/* --- MemorySize is base for :
 * TotalVisibleMemorySize,  FreePhysicalMemory, 
 * SizeStoredInPagingFiles, FreeSpaceInPagingFiles,
 * TotalVirtualMemorySize,  FreeVirtualMemory,
 * UsedPhysicalMemory, UsedVirtualMemory
 *  --- */
static MetricCalculator metricCalcMemorySize;

static MetricCalculator metricCalcTotalPhysMem;
static MetricCalculator metricCalcFreePhysMem;
static MetricCalculator metricCalcTotalSwapMem;
static MetricCalculator metricCalcFreeSwapMem;
static MetricCalculator metricCalcTotalVirtMem;
static MetricCalculator metricCalcFreeVirtMem;

static MetricCalculator metricCalcUsedPhysMem;
static MetricCalculator metricCalcUsedVirtMem;

/* --- PageInCounter, PageInRate --- */
static MetricCalculator metricCalcPageInCounter;
static MetricCalculator metricCalcPageInRate;

/* --- PageOutCounter, PageOutRate --- */
static MetricCalculator metricCalcPageOutCounter;
static MetricCalculator metricCalcPageOutRate;

/* --- LoadCounter, LoadAverage --- */
static MetricCalculator metricCalcLoadCounter;
static MetricCalculator metricCalcLoadAverage;

/* --- ContextSwitchCounter, ContextSwitchRate --- */
static MetricCalculator metricCalcContextSwitchCounter;
static MetricCalculator metricCalcContextSwitchRate;

/* --- HardwareInterruptCounter, HardwareInterruptRate --- */
static MetricCalculator metricCalcHardwareInterruptCounter;
static MetricCalculator metricCalcHardwareInterruptRate;


/* unit definitions */
static char * muMilliSeconds = "Milliseconds";
static char * muPagesPerSecond = "Pages per second";
static char * muKiloBytes = "Kilobytes";
static char * muPercent = "Percent";
static char * muUsers = "Users";
static char * muProcesses = "Processes";
static char * muQueueLength = "Queue length";
static char * muIndex = "Index";
static char * muEventsPerSecond = "Events per second";
static char * muNa ="N/A";

/* ---------------------------------------------------------------------------*/

static unsigned long long os_getCPUUserTime( char * data );
#ifdef NOTNOW
static unsigned long long os_getCPUNiceTime( char * data );
#endif
static unsigned long long os_getCPUKernelTime( char * data );
static unsigned long long os_getCPUIdleTime( char * data );
static unsigned long long os_getCPUIOWaitTime( char * data );
static unsigned long long os_getCPUTotalTime( char * data );
static float os_getCPUKernelTimePercentage( char * start, char * end ); 
static float os_getCPUUserTimePercentage( char * start, char * end );
static float os_getCPUIdleTimePercentage( char * start, char * end );
static float os_getTotalCPUTimePercentage( char * start, char * end );
static float os_getCPUConsumptionIndex( char * start, char * end );

/* ---------------------------------------------------------------------------*/

int _DefinedRepositoryMetrics( MetricRegisterId *mr,
			       const char *pluginname,
			       size_t *mcnum,
			       MetricCalculationDefinition **mc ) {
#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving metric calculation definitions\n",
	  __FILE__,__LINE__);
#endif
  if (mr==NULL||mcnum==NULL||mc==NULL) {
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
    return -1;
  }

  metricCalcDef[0].mcVersion=MD_VERSION;
  metricCalcDef[0].mcName="NumberOfUsers";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[0].mcChangeType=MD_GAUGE;
  metricCalcDef[0].mcIsContinuous=MD_TRUE;
  metricCalcDef[0].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[0].mcDataType=MD_UINT32;
  metricCalcDef[0].mcCalc=metricCalcNumOfUser;
  metricCalcDef[0].mcUnits=muUsers;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="NumberOfProcesses";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[1].mcDataType=MD_UINT32;
  metricCalcDef[1].mcCalc=metricCalcNumOfProc;
  metricCalcDef[1].mcUnits=muProcesses;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="CPUTime";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[2].mcIsContinuous=MD_FALSE;
  metricCalcDef[2].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[2].mcDataType=MD_STRING;
  metricCalcDef[2].mcCalc=metricCalcCPUTime;
  metricCalcDef[2].mcUnits=muNa;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="KernelModeTime";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_SUMMABLE;
  metricCalcDef[3].mcDataType=MD_UINT64;
  metricCalcDef[3].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[3].mcCalc=metricCalcKernelTime;
  metricCalcDef[3].mcUnits=muMilliSeconds;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="UserModeTime";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_SUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[4].mcCalc=metricCalcUserTime;
  metricCalcDef[4].mcUnits=muMilliSeconds;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="TotalCPUTime";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_SUMMABLE;
  metricCalcDef[5].mcDataType=MD_UINT64;
  metricCalcDef[5].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[5].mcCalc=metricCalcTotalCPUTime;
  metricCalcDef[5].mcUnits=muMilliSeconds;

  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="MemorySize";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[6].mcIsContinuous=MD_FALSE;
  metricCalcDef[6].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[6].mcDataType=MD_STRING;
  metricCalcDef[6].mcCalc=metricCalcMemorySize;
  metricCalcDef[6].mcUnits=muNa;

  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="TotalVisibleMemorySize";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_UINT64;
  metricCalcDef[7].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[7].mcCalc=metricCalcTotalPhysMem;
  metricCalcDef[7].mcUnits=muKiloBytes;

  metricCalcDef[8].mcVersion=MD_VERSION;
  metricCalcDef[8].mcName="FreePhysicalMemory";
  metricCalcDef[8].mcId=mr(pluginname,metricCalcDef[8].mcName);
  metricCalcDef[8].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[8].mcChangeType=MD_GAUGE;
  metricCalcDef[8].mcIsContinuous=MD_TRUE;
  metricCalcDef[8].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[8].mcDataType=MD_UINT64;
  metricCalcDef[8].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[8].mcCalc=metricCalcFreePhysMem;
  metricCalcDef[8].mcUnits=muKiloBytes;

  metricCalcDef[9].mcVersion=MD_VERSION;
  metricCalcDef[9].mcName="SizeStoredInPagingFiles";
  metricCalcDef[9].mcId=mr(pluginname,metricCalcDef[9].mcName);
  metricCalcDef[9].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[9].mcChangeType=MD_GAUGE;
  metricCalcDef[9].mcIsContinuous=MD_TRUE;
  metricCalcDef[9].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[9].mcDataType=MD_UINT64;
  metricCalcDef[9].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[9].mcCalc=metricCalcTotalSwapMem;
  metricCalcDef[9].mcUnits=muKiloBytes;

  metricCalcDef[10].mcVersion=MD_VERSION;
  metricCalcDef[10].mcName="FreeSpaceInPagingFiles";
  metricCalcDef[10].mcId=mr(pluginname,metricCalcDef[10].mcName);
  metricCalcDef[10].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[10].mcChangeType=MD_GAUGE;
  metricCalcDef[10].mcIsContinuous=MD_TRUE;
  metricCalcDef[10].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[10].mcDataType=MD_UINT64;
  metricCalcDef[10].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[10].mcCalc=metricCalcFreeSwapMem;
  metricCalcDef[10].mcUnits=muKiloBytes;

  metricCalcDef[11].mcVersion=MD_VERSION;
  metricCalcDef[11].mcName="TotalVirtualMemorySize";
  metricCalcDef[11].mcId=mr(pluginname,metricCalcDef[11].mcName);
  metricCalcDef[11].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[11].mcChangeType=MD_GAUGE;
  metricCalcDef[11].mcIsContinuous=MD_TRUE;
  metricCalcDef[11].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[11].mcDataType=MD_UINT64;
  metricCalcDef[11].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[11].mcCalc=metricCalcTotalVirtMem;
  metricCalcDef[11].mcUnits=muKiloBytes;

  metricCalcDef[12].mcVersion=MD_VERSION;
  metricCalcDef[12].mcName="FreeVirtualMemory";
  metricCalcDef[12].mcId=mr(pluginname,metricCalcDef[12].mcName);
  metricCalcDef[12].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[12].mcChangeType=MD_GAUGE;
  metricCalcDef[12].mcIsContinuous=MD_TRUE;
  metricCalcDef[12].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[12].mcDataType=MD_UINT64;
  metricCalcDef[12].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[12].mcCalc=metricCalcFreeVirtMem;
  metricCalcDef[12].mcUnits=muKiloBytes;

  metricCalcDef[13].mcVersion=MD_VERSION;
  metricCalcDef[13].mcName="PageInCounter";
  metricCalcDef[13].mcId=mr(pluginname,metricCalcDef[13].mcName);
  metricCalcDef[13].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[13].mcChangeType=MD_COUNTER;
  metricCalcDef[13].mcIsContinuous=MD_TRUE;
  metricCalcDef[13].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[13].mcDataType=MD_UINT64;
  metricCalcDef[13].mcCalc=metricCalcPageInCounter;
  metricCalcDef[13].mcUnits=muNa;

  metricCalcDef[14].mcVersion=MD_VERSION;
  metricCalcDef[14].mcName="PageInRate";
  metricCalcDef[14].mcId=mr(pluginname,metricCalcDef[14].mcName);
  metricCalcDef[14].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[14].mcChangeType=MD_GAUGE;
  metricCalcDef[14].mcIsContinuous=MD_TRUE;
  metricCalcDef[14].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[14].mcDataType=MD_UINT64;
  metricCalcDef[14].mcAliasId=metricCalcDef[13].mcId;
  metricCalcDef[14].mcCalc=metricCalcPageInRate;
  metricCalcDef[14].mcUnits=muPagesPerSecond;

  metricCalcDef[15].mcVersion=MD_VERSION;
  metricCalcDef[15].mcName="LoadCounter";
  metricCalcDef[15].mcId=mr(pluginname,metricCalcDef[15].mcName);
  metricCalcDef[15].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[15].mcChangeType=MD_COUNTER;
  metricCalcDef[15].mcIsContinuous=MD_TRUE;
  metricCalcDef[15].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[15].mcDataType=MD_FLOAT32;
  metricCalcDef[15].mcCalc=metricCalcLoadCounter;
  metricCalcDef[15].mcUnits=muNa;

  metricCalcDef[16].mcVersion=MD_VERSION;
  metricCalcDef[16].mcName="LoadAverage";
  metricCalcDef[16].mcId=mr(pluginname,metricCalcDef[16].mcName);
  metricCalcDef[16].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_AVERAGE;
  metricCalcDef[16].mcChangeType=MD_GAUGE;
  metricCalcDef[16].mcIsContinuous=MD_TRUE;
  metricCalcDef[16].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[16].mcDataType=MD_FLOAT32;
  metricCalcDef[16].mcAliasId=metricCalcDef[15].mcId;
  metricCalcDef[16].mcCalc=metricCalcLoadAverage;
  metricCalcDef[16].mcUnits=muQueueLength;

  metricCalcDef[17].mcVersion=MD_VERSION;
  metricCalcDef[17].mcName="InternalViewKernelModePercentage";
  metricCalcDef[17].mcId=mr(pluginname,metricCalcDef[17].mcName);
  metricCalcDef[17].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[17].mcChangeType=MD_GAUGE;
  metricCalcDef[17].mcIsContinuous=MD_TRUE;
  metricCalcDef[17].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[17].mcDataType=MD_FLOAT32;
  metricCalcDef[17].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[17].mcCalc=metricCalcInternKernelTimePerc;
  metricCalcDef[17].mcUnits=muPercent;

  metricCalcDef[18].mcVersion=MD_VERSION;
  metricCalcDef[18].mcName="InternalViewUserModePercentage";
  metricCalcDef[18].mcId=mr(pluginname,metricCalcDef[18].mcName);
  metricCalcDef[18].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[18].mcChangeType=MD_GAUGE;
  metricCalcDef[18].mcIsContinuous=MD_TRUE;
  metricCalcDef[18].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[18].mcDataType=MD_FLOAT32;
  metricCalcDef[18].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[18].mcCalc=metricCalcInternUserTimePerc;
  metricCalcDef[18].mcUnits=muPercent;

  metricCalcDef[19].mcVersion=MD_VERSION;
  metricCalcDef[19].mcName="InternalViewIdlePercentage";
  metricCalcDef[19].mcId=mr(pluginname,metricCalcDef[19].mcName);
  metricCalcDef[19].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[19].mcChangeType=MD_GAUGE;
  metricCalcDef[19].mcIsContinuous=MD_TRUE;
  metricCalcDef[19].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[19].mcDataType=MD_FLOAT32;
  metricCalcDef[19].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[19].mcCalc=metricCalcInternIdleTimePerc;
  metricCalcDef[19].mcUnits=muPercent;

  metricCalcDef[20].mcVersion=MD_VERSION;
  metricCalcDef[20].mcName="InternalViewTotalCPUPercentage";
  metricCalcDef[20].mcId=mr(pluginname,metricCalcDef[20].mcName);
  metricCalcDef[20].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[20].mcChangeType=MD_GAUGE;
  metricCalcDef[20].mcIsContinuous=MD_TRUE;
  metricCalcDef[20].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[20].mcDataType=MD_FLOAT32;
  metricCalcDef[20].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[20].mcCalc=metricCalcInternTotalCPUTimePerc;
  metricCalcDef[20].mcUnits=muPercent;

  metricCalcDef[21].mcVersion=MD_VERSION;
  metricCalcDef[21].mcName="ExternalViewKernelModePercentage";
  metricCalcDef[21].mcId=mr(pluginname,metricCalcDef[21].mcName);
  metricCalcDef[21].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[21].mcChangeType=MD_GAUGE;
  metricCalcDef[21].mcIsContinuous=MD_TRUE;
  metricCalcDef[21].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[21].mcDataType=MD_FLOAT32;
  metricCalcDef[21].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[21].mcCalc=metricCalcExternKernelTimePerc;
  metricCalcDef[21].mcUnits=muPercent;

  metricCalcDef[22].mcVersion=MD_VERSION;
  metricCalcDef[22].mcName="ExternalViewUserModePercentage";
  metricCalcDef[22].mcId=mr(pluginname,metricCalcDef[22].mcName);
  metricCalcDef[22].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[22].mcChangeType=MD_GAUGE;
  metricCalcDef[22].mcIsContinuous=MD_TRUE;
  metricCalcDef[22].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[22].mcDataType=MD_FLOAT32;
  metricCalcDef[22].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[22].mcCalc=metricCalcExternUserTimePerc;
  metricCalcDef[22].mcUnits=muPercent;

  metricCalcDef[23].mcVersion=MD_VERSION;
  metricCalcDef[23].mcName="ExternalViewTotalCPUPercentage";
  metricCalcDef[23].mcId=mr(pluginname,metricCalcDef[23].mcName);
  metricCalcDef[23].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[23].mcChangeType=MD_GAUGE;
  metricCalcDef[23].mcIsContinuous=MD_TRUE;
  metricCalcDef[23].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[23].mcDataType=MD_FLOAT32;
  metricCalcDef[23].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[23].mcCalc=metricCalcExternTotalCPUTimePerc;
  metricCalcDef[23].mcUnits=muPercent;

  metricCalcDef[24].mcVersion=MD_VERSION;
  metricCalcDef[24].mcName="PageOutCounter";
  metricCalcDef[24].mcId=mr(pluginname,metricCalcDef[24].mcName);
  metricCalcDef[24].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[24].mcChangeType=MD_COUNTER;
  metricCalcDef[24].mcIsContinuous=MD_TRUE;
  metricCalcDef[24].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[24].mcDataType=MD_UINT64;
  metricCalcDef[24].mcCalc=metricCalcPageOutCounter;
  metricCalcDef[24].mcUnits=muNa;

  metricCalcDef[25].mcVersion=MD_VERSION;
  metricCalcDef[25].mcName="PageOutRate";
  metricCalcDef[25].mcId=mr(pluginname,metricCalcDef[25].mcName);
  metricCalcDef[25].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[25].mcChangeType=MD_GAUGE;
  metricCalcDef[25].mcIsContinuous=MD_TRUE;
  metricCalcDef[25].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[25].mcDataType=MD_UINT64;
  metricCalcDef[25].mcAliasId=metricCalcDef[24].mcId;
  metricCalcDef[25].mcCalc=metricCalcPageOutRate;
  metricCalcDef[25].mcUnits=muPagesPerSecond;

  metricCalcDef[26].mcVersion=MD_VERSION;
  metricCalcDef[26].mcName="CPUConsumptionIndex";
  metricCalcDef[26].mcId=mr(pluginname,metricCalcDef[26].mcName);
  metricCalcDef[26].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[26].mcChangeType=MD_GAUGE;
  metricCalcDef[26].mcIsContinuous=MD_TRUE;
  metricCalcDef[26].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[26].mcDataType=MD_FLOAT32;
  metricCalcDef[26].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[26].mcCalc=metricCalcCPUConsumptionIndex;
  metricCalcDef[26].mcUnits=muIndex;

  metricCalcDef[27].mcVersion=MD_VERSION;
  metricCalcDef[27].mcName="ContextSwitchCounter";
  metricCalcDef[27].mcId=mr(pluginname,metricCalcDef[27].mcName);
  metricCalcDef[27].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[27].mcChangeType=MD_COUNTER;
  metricCalcDef[27].mcIsContinuous=MD_TRUE;
  metricCalcDef[27].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[27].mcDataType=MD_UINT64;
  metricCalcDef[27].mcCalc=metricCalcContextSwitchCounter;
  metricCalcDef[27].mcUnits=muNa;

  metricCalcDef[28].mcVersion=MD_VERSION;
  metricCalcDef[28].mcName="ContextSwitchRate";
  metricCalcDef[28].mcId=mr(pluginname,metricCalcDef[28].mcName);
  metricCalcDef[28].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[28].mcChangeType=MD_GAUGE;
  metricCalcDef[28].mcIsContinuous=MD_TRUE;
  metricCalcDef[28].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[28].mcDataType=MD_UINT64;
  metricCalcDef[28].mcAliasId=metricCalcDef[27].mcId;
  metricCalcDef[28].mcCalc=metricCalcContextSwitchRate;
  metricCalcDef[28].mcUnits=muEventsPerSecond;

  metricCalcDef[29].mcVersion=MD_VERSION;
  metricCalcDef[29].mcName="HardwareInterruptCounter";
  metricCalcDef[29].mcId=mr(pluginname,metricCalcDef[29].mcName);
  metricCalcDef[29].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[29].mcChangeType=MD_COUNTER;
  metricCalcDef[29].mcIsContinuous=MD_TRUE;
  metricCalcDef[29].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[29].mcDataType=MD_UINT64;
  metricCalcDef[29].mcCalc=metricCalcHardwareInterruptCounter;
  metricCalcDef[29].mcUnits=muNa;

  metricCalcDef[30].mcVersion=MD_VERSION;
  metricCalcDef[30].mcName="HardwareInterruptRate";
  metricCalcDef[30].mcId=mr(pluginname,metricCalcDef[30].mcName);
  metricCalcDef[30].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[30].mcChangeType=MD_GAUGE;
  metricCalcDef[30].mcIsContinuous=MD_TRUE;
  metricCalcDef[30].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[30].mcDataType=MD_UINT64;
  metricCalcDef[30].mcAliasId=metricCalcDef[29].mcId;
  metricCalcDef[30].mcCalc=metricCalcHardwareInterruptRate;
  metricCalcDef[30].mcUnits=muEventsPerSecond;

  metricCalcDef[31].mcVersion=MD_VERSION;
  metricCalcDef[31].mcName="UsedPhysicalMemory";
  metricCalcDef[31].mcId=mr(pluginname,metricCalcDef[31].mcName);
  metricCalcDef[31].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[31].mcChangeType=MD_GAUGE;
  metricCalcDef[31].mcIsContinuous=MD_TRUE;
  metricCalcDef[31].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[31].mcDataType=MD_UINT64;
  metricCalcDef[31].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[31].mcCalc=metricCalcUsedPhysMem;
  metricCalcDef[31].mcUnits=muKiloBytes;
  
  metricCalcDef[32].mcVersion=MD_VERSION;
  metricCalcDef[32].mcName="UsedVirtualMemory";
  metricCalcDef[32].mcId=mr(pluginname,metricCalcDef[32].mcName);
  metricCalcDef[32].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT;
  metricCalcDef[32].mcChangeType=MD_GAUGE;
  metricCalcDef[32].mcIsContinuous=MD_TRUE;
  metricCalcDef[32].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[32].mcDataType=MD_UINT64;
  metricCalcDef[32].mcAliasId=metricCalcDef[6].mcId;
  metricCalcDef[32].mcCalc=metricCalcUsedVirtMem;
  metricCalcDef[32].mcUnits=muKiloBytes;

  metricCalcDef[33].mcVersion=MD_VERSION;
  metricCalcDef[33].mcName="IOWaitTime";
  metricCalcDef[33].mcId=mr(pluginname,metricCalcDef[33].mcName);
  metricCalcDef[33].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[33].mcChangeType=MD_GAUGE;
  metricCalcDef[33].mcIsContinuous=MD_TRUE;
  metricCalcDef[33].mcCalculable=MD_SUMMABLE;
  metricCalcDef[33].mcDataType=MD_UINT64;
  metricCalcDef[33].mcAliasId=metricCalcDef[2].mcId;
  metricCalcDef[33].mcCalc=metricCalcIOWaitTime;
  metricCalcDef[33].mcUnits=muMilliSeconds;

  *mcnum=34;
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* NumberOfUsers                                                              */
/* ---------------------------------------------------------------------------*/

size_t metricCalcNumOfUser( MetricValue *mv,  
			    int mnum,
			    void *v, 
			    size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate NumberOfUsers\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned*)v = ntohl(*(unsigned*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* NumberOfProcesses                                                          */
/* ---------------------------------------------------------------------------*/

size_t metricCalcNumOfProc( MetricValue *mv,   
			    int mnum,
			    void *v, 
			    size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate NumberOfProcesses\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned*)v = ntohl(*(unsigned*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* CPUTime                                                                    */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data CPUTime has the following syntax :
 *
 * <user mode>:<user mode with low priority(nice)>:<system mode>:<idle task>:
 * <iowait>:<irq>:<softirq>:<steal time>:<guest time>
 *
 * the values in CPUTime are saved in Jiffies ( 1/100ths of a second )
 */

size_t metricCalcCPUTime( MetricValue *mv,   
			  int mnum,
			  void *v, 
			  size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate CPUTime\n",
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
/* KernelModeTime                                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcKernelTime( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) {

  unsigned long long kt = 0;
  unsigned long long k1 = 0;
  unsigned long long k2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate KernelModeTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * KernelModeTime is based on the third entry of the CPUTime value
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    k1 = os_getCPUKernelTime(mv[0].mvData);
    if( mnum > 1 ) {
      k2 = os_getCPUKernelTime(mv[mnum-1].mvData);
      kt = k1-k2;
    }
    else { kt = k1; }

    //fprintf(stderr,"kernel time: %lld\n",kt);
    memcpy(v,&kt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* UserModeTime                                                               */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUserTime( MetricValue *mv,   
			   int mnum,
			   void *v, 
			   size_t vlen ) {
  unsigned long long ut = 0;
  unsigned long long u1 = 0;
  unsigned long long u2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate UserModeTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * UserModeTime is based on the first entry of the CPUTime value
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    u1 = os_getCPUUserTime(mv[0].mvData);
    if( mnum > 1 ) {
      u2 = os_getCPUUserTime(mv[mnum-1].mvData);
      ut = u1-u2;
    }
    else { ut = u1; }

    //fprintf(stderr,"user time: %lld\n",ut);
    memcpy(v,&ut,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* IOWaitTime                                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcIOWaitTime( MetricValue *mv,
			     int mnum,
			     void *v,
			     size_t vlen ) {

  unsigned long long iot = 0;
  unsigned long long io1 = 0;
  unsigned long long io2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate IOWaitTime\n",
	  __FILE__,__LINE__);
#endif
  /*
   * IOWaitTime is based on the fith entry of the CPUTime value
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    io1 = os_getCPUIOWaitTime(mv[0].mvData);
    if( mnum > 1 ) {
      io2 = os_getCPUIOWaitTime(mv[mnum-1].mvData);
      iot = io1-io2;
    }
    else { iot = io1; }

    memcpy(v,&iot,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* TotalCPUTime                                                               */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalCPUTime( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) {
  unsigned long long t1    = 0;
  unsigned long long t2    = 0;
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate TotalCPUTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * TotalCPUTime is the sum of UserModeTime and KernelModeTime
   *
   */

  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    t1 = os_getCPUTotalTime(mv[0].mvData);
    if( mnum > 1 ) {
      t2 = os_getCPUTotalTime(mv[mnum-1].mvData);
      total = t1-t2;
    }
    else { total = t1; }
    
    //fprintf(stderr,"total time: %lld\n",total);
    memcpy(v,&total,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* MemorySize                                                                 */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data MemorySize has the following syntax :
 *
 * <TotalVisibleMemorySize>:<FreePhysicalMemory>:<SizeStoredInPagingFiles>:<FreeSpaceInPagingFiles>
 *
 * the values in MemorySize are saved in kBytes
 */

size_t metricCalcMemorySize( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate MemorySize\n",
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
/* TotalVisibleMemorySize                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalPhysMem( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) { 
  char             * hlp  = NULL;
  char             * end  = NULL;
  char             * pmem = NULL;
  unsigned long long mem  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate TotalVisibleMemorySize\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the TotalVisibleMemorySize is based on the first entry of the 
   * MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = mv->mvData;
    end = strchr(hlp, ':');

    pmem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(pmem, hlp, (strlen(hlp)-strlen(end)) );
    mem = strtoll(pmem,(char**)NULL,10);
    free(pmem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* FreePhysicalMemory                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcFreePhysMem( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) { 
  char             * hlp  = NULL;
  char             * end  = NULL;
  char             * pmem = NULL;
  unsigned long long mem  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate FreePhysicalMemory\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the FreePhysicalMemory is based on the second entry of the 
   * MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = strchr(mv->mvData, ':');
    hlp++;
    end = strchr(hlp, ':');

    pmem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(pmem, hlp, (strlen(hlp)-strlen(end)) );
    mem = strtoll(pmem,(char**)NULL,10);
    free(pmem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* UsedPhysicalMemory                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUsedPhysMem(MetricValue *mv,
							int mnum,
							void *v,
							size_t vlen)
{
	char * hlp = NULL;
	char * end = NULL;
	char * pmem = NULL;
	char * smem = NULL;
	unsigned long long usedmem;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate UsedPhysicalMemory\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the UsedPhysicalMemory is based on the difference of the first and second 
   * entries of the MemorySize value
   *
   */
	
	if (mv && (vlen >= sizeof(unsigned long long)) && (mnum==1)) {
		hlp = mv->mvData;
		end = strchr(hlp, ':');
		
		pmem = calloc(1, (strlen(hlp) - strlen(end) + 1));
		strncpy(pmem, hlp, (strlen(hlp) - strlen(end)));

		hlp = ++end;
		end = strchr(hlp, ':');
		
		smem = calloc(1, (strlen(hlp) - strlen(end) + 1));
		strncpy(smem, hlp, (strlen(hlp) - strlen(end)));
		
		usedmem = strtoll(pmem, (char**)NULL, 10) - strtoll(smem, (char**)NULL, 10);
		free(pmem);
		free(smem);
		
		memcpy(v, &usedmem, sizeof(unsigned long long));
		return sizeof(unsigned long long);
	}
	return -1;
}

/* ---------------------------------------------------------------------------*/
/* SizeStoredInPagingFiles                                                    */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalSwapMem( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) { 
  char             * hlp  = NULL;
  char             * end  = NULL;
  char             * pmem = NULL;
  unsigned long long mem  = 0;
  int                i    = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate SizeStoredInPagingFiles\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the SizeStoredInPagingFiles is based on the third entry of the 
   * MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = mv->mvData;
    for(;i<2;i++) {
      hlp = strchr(hlp, ':');
      hlp++;
    }
    end = strchr(hlp, ':');

    pmem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(pmem, hlp, (strlen(hlp)-strlen(end)) );
    mem = strtoll(pmem,(char**)NULL,10);
    free(pmem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* FreeSpaceInPagingFiles                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcFreeSwapMem( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) { 
  char             * hlp  = NULL;
  char             * pmem = NULL;
  unsigned long long mem  = 0;
  int                i    = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate FreeSpaceInPagingFiles\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the FreeSpaceInPagingFiles is based on the last entry of the 
   * MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = mv->mvData;
    for(;i<3;i++) {
      hlp = strchr(hlp, ':');
      hlp++;
    }

    pmem = calloc(1, (strlen(hlp)+1) );
    strcpy(pmem, hlp );
    mem = strtoll(pmem,(char**)NULL,10);
    free(pmem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* TotalVirtualMemorySize                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalVirtMem( MetricValue *mv,  
			       int mnum, 
			       void *v, 
			       size_t vlen ) { 
  char             * hlp  = NULL;
  char             * end  = NULL;
  char             * pmem = NULL;
  char             * smem = NULL;
  unsigned long long mem  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate TotalVirtualMemorySize\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the TotalVirtualMemorySize is based on the first and the third 
   * entry of the MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = mv->mvData;
    end = strchr(hlp, ':');
    pmem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(pmem, hlp, (strlen(hlp)-strlen(end)) );

    hlp = strchr(end+1, ':');
    hlp++;
    end = strchr(hlp, ':');
    smem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(smem, hlp, (strlen(hlp)-strlen(end)) );

    mem = strtoll(pmem,(char**)NULL,10) + strtoll(smem,(char**)NULL,10);
    free(pmem);
    free(smem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/*  FreeVirtualMemory                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcFreeVirtMem( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) { 
  char             * hlp  = NULL;
  char             * end  = NULL;
  char             * pmem = NULL;
  char             * smem = NULL;
  unsigned long long mem  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate FreeVirtualMemory\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the FreeVirtualMemory is based on the second and the last entry
   * of the MemorySize value
   *
   */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum==1) ) {

    hlp = strchr(mv->mvData, ':');
    hlp++;
    end = strchr(hlp, ':');
    pmem = calloc(1, (strlen(hlp)-strlen(end)+1) );
    strncpy(pmem, hlp, (strlen(hlp)-strlen(end)) );

    hlp = strchr(end+1, ':');
    hlp++;
    smem = calloc(1, (strlen(hlp)+1) );
    strcpy(smem, hlp );

    mem = strtoll(pmem,(char**)NULL,10) + strtoll(smem,(char**)NULL,10);
    free(pmem);
    free(smem);

    memcpy(v,&mem,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* UsedVirtualMemory                                                          */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUsedVirtMem(MetricValue *mv,
							 int mnum,
							 void *v,
							 size_t vlen)
{
	char * hlp = NULL;
	char * end = NULL;
	char * pmem = NULL;
	char * smem = NULL;
	unsigned long long physmem, virtmem, usedmem;
	
#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate UsedVirtualMemory\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the UsedVirtualMemory is based on the sum of the first and third entries
   * subtracted from the sum of the second and fourth entires of the
   * MemorySize value
   *
   */
	
	if (mv && (vlen >= sizeof(unsigned long long)) && (mnum == 1)) {
		hlp = mv->mvData;
		end = strchr(hlp, ':');
		pmem = calloc(1, (strlen(hlp) - strlen(end) + 1));
		strncpy(pmem, hlp, (strlen(hlp) - strlen(end)));
		
		hlp = ++end;
		end = strchr(hlp, ':');
		smem = calloc(1, (strlen(hlp) - strlen(end) + 1));
		strncpy(smem, hlp, (strlen(hlp) - strlen(end)));
		
		physmem = strtoll(pmem, (char **)NULL, 10) - strtoll(smem, (char **)NULL, 10);
		
		hlp = ++end;
		end = strchr(hlp, ':');
		pmem = realloc(pmem, (strlen(hlp) - strlen(end) + 1));
		strncpy(pmem, hlp, (strlen(hlp) - strlen(end)));
		
		hlp = ++end;
		smem = realloc(smem, (strlen(hlp) + 1));
		strncpy(smem, hlp, strlen(hlp));

		virtmem = strtoll(pmem, (char **)NULL, 10) - strtoll(smem, (char **)NULL, 10);
		free(pmem);
		free(smem);
		
		usedmem = physmem + virtmem;
		memcpy(v, &usedmem, sizeof(unsigned long long));
		return sizeof(unsigned long long);
	}
	return -1;
}

/* ---------------------------------------------------------------------------*/
/* PageInCounter                                                              */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPageInCounter( MetricValue *mv,   
				int mnum,
				void *v, 
				size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PageInCounter\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned long long*)v = ntohll(*(unsigned long long*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageInRate                                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPageInRate( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) {
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PageInRate\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
    total = (*(unsigned long long*)mv[0].mvData - *(unsigned long long*)mv[mnum-1].mvData) / 
            (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);
    memcpy(v, &total, sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageOutCounter                                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPageOutCounter( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PageOutCounter\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned long long*)v = ntohll(*(unsigned long long*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageOutRate                                                                */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPageOutRate( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PageOutRate\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
    total = (*(unsigned long long*)mv[0].mvData - *(unsigned long long*)mv[mnum-1].mvData) / 
            (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);
    memcpy(v, &total, sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* LoadCounter                                                                */
/* ---------------------------------------------------------------------------*/

size_t metricCalcLoadCounter( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate LoadCounter\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(float*)v = ntohf(*(float*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* LoadAverage                                                                */
/* ---------------------------------------------------------------------------*/

size_t metricCalcLoadAverage( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {
  int   i     = 0;
  float total = 0;
  float sum   = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate LoadAverage\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {
    for(;i<mnum;i++) { sum = sum + ntohf(*(float*)mv[i].mvData); }
    total = sum / mnum;
    memcpy(v, &total, sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ContextSwitchCounter                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcContextSwitchCounter( MetricValue *mv,   
				       int mnum,
				       void *v, 
				       size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ContextSwitchCounter\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned long long*)v = ntohll(*(unsigned long long*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ContextSwitchRate                                                          */
/* ---------------------------------------------------------------------------*/

size_t metricCalcContextSwitchRate( MetricValue *mv,   
				    int mnum,
				    void *v, 
				    size_t vlen ) {
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ContextSwitchRate\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
    total = (ntohll(*(unsigned long long*)mv[0].mvData) - 
	     ntohll(*(unsigned long long*)mv[mnum-1].mvData)) / 
      (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);
    memcpy(v, &total, sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* HardwareInterruptCounter                                                   */
/* ---------------------------------------------------------------------------*/

size_t metricCalcHardwareInterruptCounter( MetricValue *mv,   
					   int mnum,
					   void *v, 
					   size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate HardwareInterruptCounter\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    *(unsigned long long*)v = ntohll(*(unsigned long long*)v);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* HardwareInterruptRate                                                      */
/* ---------------------------------------------------------------------------*/

size_t metricCalcHardwareInterruptRate( MetricValue *mv,   
					int mnum,
					void *v, 
					size_t vlen ) {
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate HardwareInterruptRate\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=2) ) {
    total = (htonll(*(unsigned long long*)mv[0].mvData) - 
	     htonll(*(unsigned long long*)mv[mnum-1].mvData)) / 
      (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);
    memcpy(v, &total, sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* InternalViewKernelModePercentage                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcInternKernelTimePerc( MetricValue *mv,   
				       int mnum,
				       void *v, 
				       size_t vlen ) {

  float kp = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate InternalViewKernelModePercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * InternalViewKernelModePercentage is based on KernelModeTime and
   * TotalCPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      kp = os_getCPUKernelTimePercentage( mv[mnum-1].mvData, 
					  mv[0].mvData);
    }
    else {
      kp = os_getCPUKernelTimePercentage( NULL, 
					  mv[0].mvData);
    }

    //fprintf(stderr,"kernel time percentage: %f\n",kp);
    memcpy(v,&kp,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* InternalViewUserModePercentage                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcInternUserTimePerc( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) {

  float up = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate InternalViewUserModePercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * InternalViewKernelModePercentage is based on KernelModeTime and
   * TotalCPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      up = os_getCPUUserTimePercentage( mv[mnum-1].mvData, 
					mv[0].mvData);
    }
    else {
      up = os_getCPUUserTimePercentage( NULL, 
					mv[0].mvData);
    }

    //fprintf(stderr,"user time percentage: %f\n",up);
    memcpy(v,&up,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* InternalViewIdlePercentage                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcInternIdleTimePerc( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) {
  float ip = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate InternalViewIdlePercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * InternalViewIdlePercentage is based on the fourth entry of the
   * CPUTime value and the total CPUTime (user plus kernel time)
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      ip = os_getCPUIdleTimePercentage( mv[mnum-1].mvData, 
					mv[0].mvData);
    }
    else {
      ip = os_getCPUIdleTimePercentage( NULL, 
					mv[0].mvData);
    }

    //fprintf(stderr,"idle time percentage: %f\n",ip);
    memcpy(v,&ip,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* InternalViewTotalCPUPercentage                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcInternTotalCPUTimePerc( MetricValue *mv,   
					 int mnum,
					 void *v, 
					 size_t vlen ) {

  float tp = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate InternalViewTotalCPUPercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * InternalViewTotalCPUPercentage is the difference of
   * InternalViewIdleTimePercentage to 100 %
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      tp = os_getTotalCPUTimePercentage( mv[mnum-1].mvData, 
					 mv[0].mvData);
    }
    else {
      tp = os_getTotalCPUTimePercentage( NULL, 
					 mv[0].mvData);
    }

    //fprintf(stderr,"total cpu percentage: %f\n",tp);
    memcpy(v,&tp,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ExternalViewKernelModePercentage                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcExternKernelTimePerc( MetricValue *mv,   
				       int mnum,
				       void *v, 
				       size_t vlen ) {

  float kp = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ExternalViewKernelModePercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * ExternalViewKernelModePercentage is based on KernelModeTime and
   * TotalCPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      kp = os_getCPUKernelTimePercentage( mv[mnum-1].mvData, 
					  mv[0].mvData);
    }
    else {
      kp = os_getCPUKernelTimePercentage( NULL, 
					  mv[0].mvData);
    }

    //fprintf(stderr,"external kernel time percentage: %f\n",kp);
    memcpy(v,&kp,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ExternalViewUserModePercentage                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcExternUserTimePerc( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) {

  float up = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ExternalViewUserModePercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * ExternalViewKernelModePercentage is based on KernelModeTime and
   * TotalCPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      up = os_getCPUUserTimePercentage( mv[mnum-1].mvData, 
					mv[0].mvData);
    }
    else {
      up = os_getCPUUserTimePercentage( NULL, 
					mv[0].mvData);
    }

    //fprintf(stderr,"external user time percentage: %f\n",up);
    memcpy(v,&up,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ExternalViewTotalCPUPercentage                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcExternTotalCPUTimePerc( MetricValue *mv,   
					 int mnum,
					 void *v, 
					 size_t vlen ) {

  float tp = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ExternalViewTotalCPUPercentage\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * ExternalViewTotalCPUPercentage is the difference of
   * ExternalViewIdleTimePercentage to 100 %
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      tp = os_getTotalCPUTimePercentage( mv[mnum-1].mvData, 
					 mv[0].mvData);
    }
    else {
      tp = os_getTotalCPUTimePercentage( NULL, 
					 mv[0].mvData);
    }

    //fprintf(stderr,"external total cpu percentage: %f\n",tp);
    memcpy(v,&tp,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* CPUConsumptionIndex                                                        */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCPUConsumptionIndex( MetricValue *mv,   
				      int mnum,
				      void *v, 
				      size_t vlen ) {
  float index = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate CPUConsumptionIndex\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * CPUConsumptionIndex is based on the user, kernel and idle
   * CPUTime values
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(float)) && (mnum>=1) ) {

    if(mnum>1) {
      index = os_getCPUConsumptionIndex( mv[mnum-1].mvData, 
					 mv[0].mvData);
    }
    else {
      index = os_getCPUConsumptionIndex( NULL, 
					 mv[0].mvData);
    }

    //fprintf(stderr,"cpu consumption index: %f\n",index);
    memcpy(v,&index,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* tool functions on CPUTime                                                  */
/* ---------------------------------------------------------------------------*/

unsigned long long os_getCPUUserTime( char * data ) {

  char * hlp = NULL;
  char   time[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    memset(time,0,sizeof(time));
    strncpy(time, data, (strlen(data)-strlen(hlp)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

#ifdef NOTNOW

unsigned long long os_getCPUNiceTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

#endif

unsigned long long os_getCPUKernelTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

unsigned long long os_getCPUIdleTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
	memset(time,0,sizeof(time));
	strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
	val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

unsigned long long os_getCPUIOWaitTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}


unsigned long long os_getCPUTotalTime( char * data ) {

  unsigned long long val = 0;

  val = os_getCPUUserTime(data) +
        os_getCPUKernelTime(data);

  return val;
}

/* ---------------------------------------------------------------------------*/

float os_getCPUKernelTimePercentage( char * start, char * end ) {

  float end_kernel   = 0;
  float end_total    = 0;
  float start_kernel = 0;
  float start_total  = 0;
  float kernelPerc   = 0;

  if(!end) return -1;

  end_kernel = os_getCPUKernelTime(end);
  /* CPUTotalTime is the sum of UserTime and KernelTime */
  end_total  = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_kernel = os_getCPUKernelTime(start);
    start_total  = os_getCPUTotalTime(start);

    kernelPerc = ( (end_kernel-start_kernel) / 
		   (end_total-start_total) ) *
                 os_getTotalCPUTimePercentage(start,end);
  }
  else { kernelPerc = (end_kernel / end_total ) *
	               os_getTotalCPUTimePercentage(NULL,end); }

  return kernelPerc;
}


float os_getCPUUserTimePercentage( char * start, char * end ) {

  float end_user    = 0;
  float end_total   = 0;
  float start_user  = 0;
  float start_total = 0;
  float userPerc    = 0;

  if(!end) return -1;

  end_user  = os_getCPUUserTime(end);
  /* CPUTotalTime is the sum of UserTime and KernelTime */
  end_total = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_user  = os_getCPUUserTime(start);
    start_total = os_getCPUTotalTime(start);

    userPerc = ( (end_user-start_user) / 
		 (end_total-start_total) ) *
               os_getTotalCPUTimePercentage(start,end);
  }
  else { userPerc = (end_user / end_total) *
	            os_getTotalCPUTimePercentage(NULL,end); }

  return userPerc;
}


float os_getCPUIdleTimePercentage( char * start, char * end ) {

  float end_idle    = 0;
  float end_total   = 0;
  float start_idle  = 0;
  float start_total = 0;
  float idlePerc    = 0;

  if(!end) return -1;

  end_idle  = os_getCPUIdleTime(end);
  end_total = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_idle = os_getCPUIdleTime(start);
    start_total = os_getCPUTotalTime(start);

    idlePerc = ((end_idle - start_idle) / 
		((end_idle+end_total)-(start_idle+start_total)) ) * 100;
    if(idlePerc<0) { idlePerc = 0; }
  }
  else { idlePerc = (end_idle/(end_idle+end_total))*100; }

  return idlePerc;
}


float os_getTotalCPUTimePercentage( char * start, char * end ) {

  float idlePerc  = 0;
  float totalPerc = 0;

  if(!end) return -1;

  idlePerc = os_getCPUIdleTimePercentage(start,end);
  totalPerc = 100 - idlePerc;

  return totalPerc;
}


float os_getCPUConsumptionIndex( char * start, char * end ) {

  float end_idle    = 0;
  float end_total   = 0;
  float start_idle  = 0;
  float start_total = 0;
  float index       = 0;

  if(!end) return -1;

  end_idle  = os_getCPUIdleTime(end);
  end_total = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_idle = os_getCPUIdleTime(start);
    start_total = os_getCPUTotalTime(start);

    index = (end_total - start_total) / 
            ((end_idle+end_total)-(start_idle+start_total));
  }
  else { index = end_total/(end_idle+end_total); }

  return index;
}


/* ---------------------------------------------------------------------------*/
/*                   end of repositoryOperatingSystem.c                       */
/* ---------------------------------------------------------------------------*/
