/*
 * $Id: repositoryUnixProcess.c,v 1.11 2009/05/20 19:39:56 tyreld Exp $
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
 * Repository Plugin of the following Unix Process specific metrics :
 *
 * KernelModeTime
 * UserModeTime
 * TotalCPUTime
 * ResidentSetSize
 * PageInCounter
 * PageInRate
 * InternalViewKernelModePercentage
 * InternalViewUserModePercentage
 * InternalViewTotalCPUPercentage
 * ExternalViewKernelModePercentage
 * ExternalViewUserModePercentage
 * ExternalViewTotalCPUPercentage
 * AccumulatedKernelModeTime
 * AccumulatedUserModeTime
 * AccumulatedTotalCPUTime
 * VirtualSize
 * SharedSize
 * PageOutCounter
 * PageOutRate
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <commutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[20];

/* --- CPUTime is base for :
 * KernelModeTime, UserModeTime, TotalCPUTime,
 * InternalViewKernelModePercentage, InternalViewUserModePercentage,
 * InternalViewTotalCPUPercentage, ExternalViewKernelModePercentage,
 * ExternalViewUserModePercentage, ExternalViewTotalCPUPercentage,
 * AccumulatedKernelModeTime, AccumulatedUserModeTime,
 * AccumulatedTotalCPUTime 
 * --- */
static MetricCalculator  metricCalcCPUTime;
static MetricCalculator  metricCalcKernelTime;
static MetricCalculator  metricCalcUserTime;
static MetricCalculator  metricCalcTotalCPUTime;

static MetricCalculator metricCalcInternKernelTimePerc;
static MetricCalculator metricCalcInternUserTimePerc;
static MetricCalculator metricCalcInternTotalCPUTimePerc;

static MetricCalculator metricCalcExternKernelTimePerc;
static MetricCalculator metricCalcExternUserTimePerc;
static MetricCalculator metricCalcExternTotalCPUTimePerc;

static MetricCalculator metricCalcAccKernelTime;
static MetricCalculator metricCalcAccUserTime;
static MetricCalculator metricCalcAccTotalCPUTime;

/* --- ResidentSetSize --- */
static MetricCalculator  metricCalcResSetSize;

/* --- PageInCounter, PageInRate --- */
static MetricCalculator  metricCalcPageInCounter;
static MetricCalculator  metricCalcPageInRate;

/* --- PageOutCounter, PageOutRate --- */
static MetricCalculator  metricCalcPageOutCounter;
static MetricCalculator  metricCalcPageOutRate;

/* --- VirtualSize --- */
static MetricCalculator  metricCalcVirtualSize;

/* --- SharedSize --- */
static MetricCalculator  metricCalcSharedSize;

/* unit definitions */
static char * muMilliSeconds = "Milliseconds";
static char * muPagesPerSecond = "Pages per second";
static char * muBytes = "Bytes";
static char * muPercent = "Percent";
static char * muNa ="N/A";


/* ---------------------------------------------------------------------------*/

static unsigned long long getCPUUserTime( char * data );
static unsigned long long getCPUKernelTime( char * data );
static unsigned long long getCPUTotalTime( char * data );
static float getCPUKernelTimePercentage( char * start, char * end ); 
static float getCPUUserTimePercentage( char * start, char * end );
static float getTotalCPUTimePercentage( char * start, char * end );

/* ---------------------------------------------------------------------------*/

/* TODO : move to lib repositoryTool */

static unsigned long long os_getCPUUserTime( char * data );
static unsigned long long os_getCPUKernelTime( char * data );
static unsigned long long os_getCPUIdleTime( char * data );
static unsigned long long os_getCPUTotalTime( char * data );
static float os_getCPUIdleTimePercentage( char * start, char * end );
static float os_getTotalCPUTimePercentage( char * start, char * end );

#ifdef NOTNOW
static float os_getCPUKernelTimePercentage( char * start, char * end ); 
static float os_getCPUUserTimePercentage( char * start, char * end );
static float os_getCPUConsumptionIndex( char * start, char * end );
static unsigned long long os_getCPUNiceTime( char * data );
#endif

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
  metricCalcDef[0].mcName="CPUTime";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcCPUTime;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="KernelModeTime";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_SUMMABLE;
  metricCalcDef[1].mcDataType=MD_UINT64;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcKernelTime;
  metricCalcDef[1].mcUnits=muMilliSeconds;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="UserModeTime";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcDataType=MD_UINT64;
  metricCalcDef[2].mcCalculable=MD_SUMMABLE;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcUserTime;
  metricCalcDef[2].mcUnits=muMilliSeconds;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="TotalCPUTime";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_SUMMABLE;
  metricCalcDef[3].mcDataType=MD_UINT64;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcTotalCPUTime;
  metricCalcDef[3].mcUnits=muMilliSeconds;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="ResidentSetSize";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcCalc=metricCalcResSetSize;
  metricCalcDef[4].mcUnits=muBytes;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="PageInCounter";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[5].mcChangeType=MD_COUNTER;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[5].mcDataType=MD_UINT64;
  metricCalcDef[5].mcCalc=metricCalcPageInCounter;
  metricCalcDef[5].mcUnits=muNa;

  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="PageInRate";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_UINT64;
  metricCalcDef[6].mcAliasId=metricCalcDef[5].mcId;
  metricCalcDef[6].mcCalc=metricCalcPageInRate;
  metricCalcDef[6].mcUnits=muPagesPerSecond;

  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="InternalViewKernelModePercentage";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_FLOAT32;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcInternKernelTimePerc;
  metricCalcDef[7].mcUnits=muPercent;

  metricCalcDef[8].mcVersion=MD_VERSION;
  metricCalcDef[8].mcName="InternalViewUserModePercentage";
  metricCalcDef[8].mcId=mr(pluginname,metricCalcDef[8].mcName);
  metricCalcDef[8].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[8].mcChangeType=MD_GAUGE;
  metricCalcDef[8].mcIsContinuous=MD_TRUE;
  metricCalcDef[8].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[8].mcDataType=MD_FLOAT32;
  metricCalcDef[8].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[8].mcCalc=metricCalcInternUserTimePerc;
  metricCalcDef[8].mcUnits=muPercent;

  metricCalcDef[9].mcVersion=MD_VERSION;
  metricCalcDef[9].mcName="InternalViewTotalCPUPercentage";
  metricCalcDef[9].mcId=mr(pluginname,metricCalcDef[9].mcName);
  metricCalcDef[9].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[9].mcChangeType=MD_GAUGE;
  metricCalcDef[9].mcIsContinuous=MD_TRUE;
  metricCalcDef[9].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[9].mcDataType=MD_FLOAT32;
  metricCalcDef[9].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[9].mcCalc=metricCalcInternTotalCPUTimePerc;
  metricCalcDef[9].mcUnits=muPercent;

  metricCalcDef[10].mcVersion=MD_VERSION;
  metricCalcDef[10].mcName="ExternalViewKernelModePercentage";
  metricCalcDef[10].mcId=mr(pluginname,metricCalcDef[10].mcName);
  metricCalcDef[10].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[10].mcChangeType=MD_GAUGE;
  metricCalcDef[10].mcIsContinuous=MD_TRUE;
  metricCalcDef[10].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[10].mcDataType=MD_FLOAT32;
  metricCalcDef[10].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[10].mcCalc=metricCalcExternKernelTimePerc;
  metricCalcDef[10].mcUnits=muPercent;

  metricCalcDef[11].mcVersion=MD_VERSION;
  metricCalcDef[11].mcName="ExternalViewUserModePercentage";
  metricCalcDef[11].mcId=mr(pluginname,metricCalcDef[11].mcName);
  metricCalcDef[11].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[11].mcChangeType=MD_GAUGE;
  metricCalcDef[11].mcIsContinuous=MD_TRUE;
  metricCalcDef[11].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[11].mcDataType=MD_FLOAT32;
  metricCalcDef[11].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[11].mcCalc=metricCalcExternUserTimePerc;
  metricCalcDef[11].mcUnits=muPercent;

  metricCalcDef[12].mcVersion=MD_VERSION;
  metricCalcDef[12].mcName="ExternalViewTotalCPUPercentage";
  metricCalcDef[12].mcId=mr(pluginname,metricCalcDef[12].mcName);
  metricCalcDef[12].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[12].mcChangeType=MD_GAUGE;
  metricCalcDef[12].mcIsContinuous=MD_TRUE;
  metricCalcDef[12].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[12].mcDataType=MD_FLOAT32;
  metricCalcDef[12].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[12].mcCalc=metricCalcExternTotalCPUTimePerc;
  metricCalcDef[12].mcUnits=muPercent;

  metricCalcDef[13].mcVersion=MD_VERSION;
  metricCalcDef[13].mcName="AccumulatedKernelModeTime";
  metricCalcDef[13].mcId=mr(pluginname,metricCalcDef[13].mcName);
  metricCalcDef[13].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[13].mcChangeType=MD_COUNTER;
  metricCalcDef[13].mcIsContinuous=MD_TRUE;
  metricCalcDef[13].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[13].mcDataType=MD_UINT64;
  metricCalcDef[13].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[13].mcCalc=metricCalcAccKernelTime;
  metricCalcDef[13].mcUnits=muMilliSeconds;

  metricCalcDef[14].mcVersion=MD_VERSION;
  metricCalcDef[14].mcName="AccumulatedUserModeTime";
  metricCalcDef[14].mcId=mr(pluginname,metricCalcDef[14].mcName);
  metricCalcDef[14].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[14].mcChangeType=MD_COUNTER;
  metricCalcDef[14].mcIsContinuous=MD_TRUE;
  metricCalcDef[14].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[14].mcDataType=MD_UINT64;
  metricCalcDef[14].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[14].mcCalc=metricCalcAccUserTime;
  metricCalcDef[14].mcUnits=muMilliSeconds;

  metricCalcDef[15].mcVersion=MD_VERSION;
  metricCalcDef[15].mcName="AccumulatedTotalCPUTime";
  metricCalcDef[15].mcId=mr(pluginname,metricCalcDef[15].mcName);
  metricCalcDef[15].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[15].mcChangeType=MD_COUNTER;
  metricCalcDef[15].mcIsContinuous=MD_TRUE;
  metricCalcDef[15].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[15].mcDataType=MD_UINT64;
  metricCalcDef[15].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[15].mcCalc=metricCalcAccTotalCPUTime;
  metricCalcDef[15].mcUnits=muMilliSeconds;

  metricCalcDef[16].mcVersion=MD_VERSION;
  metricCalcDef[16].mcName="VirtualSize";
  metricCalcDef[16].mcId=mr(pluginname,metricCalcDef[16].mcName);
  metricCalcDef[16].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[16].mcChangeType=MD_GAUGE;
  metricCalcDef[16].mcIsContinuous=MD_TRUE;
  metricCalcDef[16].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[16].mcDataType=MD_UINT64;
  metricCalcDef[16].mcCalc=metricCalcVirtualSize;
  metricCalcDef[16].mcUnits=muBytes;

  metricCalcDef[17].mcVersion=MD_VERSION;
  metricCalcDef[17].mcName="SharedSize";
  metricCalcDef[17].mcId=mr(pluginname,metricCalcDef[17].mcName);
  metricCalcDef[17].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[17].mcChangeType=MD_GAUGE;
  metricCalcDef[17].mcIsContinuous=MD_TRUE;
  metricCalcDef[17].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[17].mcDataType=MD_UINT64;
  metricCalcDef[17].mcCalc=metricCalcSharedSize;
  metricCalcDef[17].mcUnits=muBytes;

  metricCalcDef[18].mcVersion=MD_VERSION;
  metricCalcDef[18].mcName="PageOutCounter";
  metricCalcDef[18].mcId=mr(pluginname,metricCalcDef[18].mcName);
  metricCalcDef[18].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[18].mcChangeType=MD_COUNTER;
  metricCalcDef[18].mcIsContinuous=MD_TRUE;
  metricCalcDef[18].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[18].mcDataType=MD_UINT64;
  metricCalcDef[18].mcCalc=metricCalcPageOutCounter;
  metricCalcDef[18].mcUnits=muNa;

  metricCalcDef[19].mcVersion=MD_VERSION;
  metricCalcDef[19].mcName="PageOutRate";
  metricCalcDef[19].mcId=mr(pluginname,metricCalcDef[19].mcName);
  metricCalcDef[19].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[19].mcChangeType=MD_GAUGE;
  metricCalcDef[19].mcIsContinuous=MD_TRUE;
  metricCalcDef[19].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[19].mcDataType=MD_UINT64;
  metricCalcDef[19].mcAliasId=metricCalcDef[18].mcId;
  metricCalcDef[19].mcCalc=metricCalcPageOutRate;
  metricCalcDef[19].mcUnits=muPagesPerSecond;

  *mcnum=20;
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* CPUTime                                                                    */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data CPUTime has the following syntax :
 *
 * <PID user mode time>:<PID kernel mode time>:<OS user mode>:
 * <OS user mode with low priority(nice)>:<OS system mode>:<OS idle task>
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
   * the KernelModeTime is based on the second entry of the CPUTime 
   * value and needs to be multiplied by 10 to get milliseconds
   *
   */

  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    k1 = getCPUKernelTime(mv[0].mvData);
    if( mnum > 1 ) {
      k2 = getCPUKernelTime(mv[mnum-1].mvData);
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
   * the UserModeTime is based on the first entry of the CPUTime 
   * value and needs to be multiplied by 10 to get milliseconds
   *
   */

  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    u1 = getCPUUserTime(mv[0].mvData);
    if( mnum > 1 ) {
      u2 = getCPUUserTime(mv[mnum-1].mvData);
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
/* TotalCPUTime                                                               */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalCPUTime( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) {
  unsigned long long t1 = 0;
  unsigned long long t2 = 0;
  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate TotalCPUTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * the TotalCPUTime is based on both entries of the CPUTime 
   * value and needs to be multiplied by 10 to get milliseconds
   *
   */

  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    t1 = getCPUTotalTime(mv[0].mvData);
    if( mnum > 1 ) {
      t2 = getCPUTotalTime(mv[mnum-1].mvData);
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
/* ResidentSetSize                                                            */
/* ---------------------------------------------------------------------------*/

size_t metricCalcResSetSize( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ResidentSetSize\n",
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
/* VirtualSize                                                                */
/* ---------------------------------------------------------------------------*/

size_t metricCalcVirtualSize( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {
  
#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate VirtualSize\n",
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
/* SharedSize                                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcSharedSize( MetricValue *mv,   
			     int mnum,
			     void *v, 
			     size_t vlen ) {
  
#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate SharedSize\n",
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
      kp = getCPUKernelTimePercentage( mv[mnum-1].mvData, 
				       mv[0].mvData);
    }
    else {
      kp = getCPUKernelTimePercentage( NULL, 
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
      up = getCPUUserTimePercentage( mv[mnum-1].mvData, 
				     mv[0].mvData);
    }
    else {
      up = getCPUUserTimePercentage( NULL, 
				     mv[0].mvData);
    }

    //fprintf(stderr,"user time percentage: %f\n",up);
    memcpy(v,&up,sizeof(float));
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
      tp = getTotalCPUTimePercentage( mv[mnum-1].mvData, 
				      mv[0].mvData);
    }
    else {
      tp = getTotalCPUTimePercentage( NULL, 
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
      kp = getCPUKernelTimePercentage( mv[mnum-1].mvData, 
				       mv[0].mvData);
    }
    else {
      kp = getCPUKernelTimePercentage( NULL, 
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
      up = getCPUUserTimePercentage( mv[mnum-1].mvData, 
				     mv[0].mvData);
    }
    else {
      up = getCPUUserTimePercentage( NULL, 
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
      tp = getTotalCPUTimePercentage( mv[mnum-1].mvData, 
				      mv[0].mvData);
    }
    else {
      tp = getTotalCPUTimePercentage( NULL, 
				      mv[0].mvData);
    }

    //fprintf(stderr,"external total cpu percentage: %f\n",tp);
    memcpy(v,&tp,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* AccumulatedKernelModeTime                                                  */
/* ---------------------------------------------------------------------------*/

size_t metricCalcAccKernelTime( MetricValue *mv,   
				int mnum,
				void *v, 
				size_t vlen ) {

  unsigned long long kt = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate AccumulatedKernelModeTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * AccumulatedKernelModeTime is the second entry of CPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    kt = getCPUKernelTime(mv[0].mvData);

    //fprintf(stderr,"accumulated kernel mode time: %f\n",kt);
    memcpy(v,&kt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* AccumulatedUserModeTime                                                    */
/* ---------------------------------------------------------------------------*/

size_t metricCalcAccUserTime( MetricValue *mv,   
			      int mnum,
			      void *v, 
			      size_t vlen ) {

  unsigned long long ut = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate AccumulatedUserModeTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * AccumulatedUserModeTime is the first entry of CPUTime
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    ut = getCPUUserTime(mv[0].mvData);

    //fprintf(stderr,"accumulated user mode time: %f\n",ut);
    memcpy(v,&ut,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* AccumulatedTotalCPUTime                                                    */
/* ---------------------------------------------------------------------------*/

size_t metricCalcAccTotalCPUTime( MetricValue *mv,   
				  int mnum,
				  void *v, 
				  size_t vlen ) {

  unsigned long long total = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate AccumulatedTotalCPUTime\n",
	  __FILE__,__LINE__);
#endif
  /* 
   * AccumulatedTotalCPUTime is the sum of 
   *
   */
  /*
  fprintf(stderr,"mnum : %i\n",mnum);
  fprintf(stderr,"mv[0].mvData : %s\n",mv[0].mvData);
  fprintf(stderr,"mv[mnum-1].mvData : %s\n",mv[mnum-1].mvData);
  */

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    total = getCPUTotalTime(mv[0].mvData);

    //fprintf(stderr,"accumulated total cpu time: %f\n",total);
    memcpy(v,&total,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* tool functions on CPUTime                                                  */
/* ---------------------------------------------------------------------------*/

unsigned long long getCPUUserTime( char * data ) {

  char * hlp = NULL;
  char   time[128];
  unsigned long long val = 0;

  /* first entry of data (CPUTime) */
  if( (hlp = strchr(data, ':')) != NULL ) {
    memset(time,0,sizeof(time));
    strncpy(time, data, (strlen(data)-strlen(hlp)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

unsigned long long getCPUKernelTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  /* second entry of data (CPUTime) */
  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

unsigned long long getCPUTotalTime( char * data ) {

  unsigned long long val = 0;

  /* sum of User and Kernel time of data (CPUTime) */
  val = getCPUUserTime(data) +
        getCPUKernelTime(data);

  return val;
}


/* ---------------------------------------------------------------------------*/

float getCPUKernelTimePercentage( char * start, char * end ) {

  float end_kernel     = 0;
  float end_total_os   = 0;
  float start_kernel   = 0;
  float start_total_os = 0;
  float kernelPerc     = 0;

  if(!end) return -1;

  end_kernel = getCPUKernelTime(end);
  end_total_os = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_kernel = getCPUKernelTime(start);
    start_total_os = os_getCPUTotalTime(start);

    if (end_total_os == start_total_os) {
      kernelPerc = 0;
    } else {
      kernelPerc = ( (end_kernel-start_kernel) / 
		     (end_total_os-start_total_os) ) *
	os_getTotalCPUTimePercentage(start,end) ;
    }
  }
  else {
    kernelPerc = (end_kernel / end_total_os) *
                 os_getTotalCPUTimePercentage(NULL,end);
  }

  return kernelPerc;
}


float getCPUUserTimePercentage( char * start, char * end ) {

  float end_user       = 0;
  float end_total_os   = 0;
  float start_user     = 0;
  float start_total_os = 0;
  float userPerc       = 0;

  if(!end) return -1;

  end_user  = getCPUUserTime(end);
  end_total_os = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_user  = getCPUUserTime(start);
    start_total_os = os_getCPUTotalTime(start);

    if (end_total_os == start_total_os) {
      userPerc = 0;
    } else {
      userPerc = ( (end_user-start_user) / 
		   (end_total_os-start_total_os) ) *
	os_getTotalCPUTimePercentage(start,end) ;
    }
  } else { 
    userPerc = (end_user / end_total_os) *
               os_getTotalCPUTimePercentage(NULL,end); 
  }

  return userPerc;
}


float getTotalCPUTimePercentage( char * start, char * end ) {

  float kernelPerc = 0;
  float userPerc   = 0;
  float totalPerc  = 0;

  if(!end) return -1;

  kernelPerc = getCPUKernelTimePercentage(start,end);
  userPerc = getCPUUserTimePercentage(start,end);
  totalPerc = kernelPerc + userPerc;

  return totalPerc;
}


/* ---------------------------------------------------------------------------*/
/* OperatingSystem specific metrics                                           */
/* ---------------------------------------------------------------------------*/

unsigned long long os_getCPUUserTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  /* third entry of data (CPUTime) */
  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(data, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, data, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

unsigned long long os_getCPUKernelTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  /* fith entry of data (CPUTime) */
  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(data, ':');
    hlp++;
    hlp = strchr(data, ':');
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
  char   time[128];
  unsigned long long val = 0;

  /* sixt and last entry of data (CPUTime) */
  if( (hlp = strrchr(data, ':')) != NULL ) {
    hlp++;
    memset(time,0,sizeof(time));
    strcpy(time, hlp);
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

    if ((end_idle+start_idle-end_total-start_total)==0) {
      idlePerc = 0;
    } else {
      idlePerc = ((end_idle - start_idle) / 
		  ((end_idle+end_total)-(start_idle+start_total)) ) * 100;
      if(idlePerc<0) { idlePerc = 0; }
    }
  } else { idlePerc = (end_idle/(end_idle+end_total))*100; }

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

#ifdef NOTNOW

unsigned long long os_getCPUNiceTime( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   time[128];
  unsigned long long val = 0;

  /* fourth entry of data (CPUTime) */
  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(data, ':');
    hlp++;
    hlp = strchr(data, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(time,0,sizeof(time));
    strncpy(time, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(time,(char**)NULL,10)*10;
  }

  return val;
}

float os_getCPUKernelTimePercentage( char * start, char * end ) {

  float end_kernel   = 0;
  float end_total    = 0;
  float start_kernel = 0;
  float start_total  = 0;
  float kernelPerc   = 0;

  if(!end) return -1;

  end_kernel = os_getCPUKernelTime(end);
  end_total  = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_kernel = os_getCPUKernelTime(start);
    start_total  = os_getCPUTotalTime(start);
    
    if (end_total=start_total) {
      kernelPerc = 0;
    } else {
      kernelPerc = ( (end_kernel-start_kernel) / 
		     (end_total-start_total) ) *
	os_getTotalCPUTimePercentage(start,end) ;
    }
  } else { kernelPerc = (end_kernel / end_total ) *
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
  end_total = os_getCPUTotalTime(end);

  if( start != NULL ) {
    start_user  = os_getCPUUserTime(start);
    start_total = os_getCPUTotalTime(start);

    if (end_total==start_total) {
      userPerc = 0;
    } else {
      userPerc = ( (end_user-start_user) / 
		   (end_total-start_total) ) *
	os_getTotalCPUTimePercentage(start,end) ;
    }
  } else { userPerc = (end_user / end_total) *
	            os_getTotalCPUTimePercentage(NULL,end); }

  return userPerc;
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

    if ((end_idle+end_total-start_idle-start_total)==0) {
      index = 0;
    } else {
      index = (end_total - start_total) / 
	((end_idle+end_total)-(start_idle+start_total));
    }
  } else { index = end_total/(end_idle+end_total); }

  return index;
}

#endif

/* ---------------------------------------------------------------------------*/
/*                       end of repositoryUnixProcess.c                       */
/* ---------------------------------------------------------------------------*/
