/*
 * $Id: repositoryzLPAR.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * Metrics Repository Plugin of the following LPAR Metrics :
 *  IBM_zComputerSystem.KernelModeTime
 *  IBM_zComputerSystem.UserModeTime
 *  IBM_zComputerSystem.TotalCPUTime
 *  IBM_zComputerSystem.UnusedPartitionCPUCapacity
 *  IBM_zComputerSystem.ExternalViewKernelModePercentage
 *  IBM_zComputerSystem.ExternalViewUserModePercentage
 *  IBM_zComputerSystem.ExternalViewTotalCPUPercentage
 *  IBM_zComputerSystem.ExternalViewIdlePercentage
 *  IBM_zComputerSystem.ActiveVirtualProcessors
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
static MetricCalculator  metricCalcLPARTimes;
static MetricCalculator  metricCalcKernelModeTime;
static MetricCalculator  metricCalcUserModeTime;
static MetricCalculator  metricCalcTotalCPUTime;
static MetricCalculator  metricCalcUnusedPartitionCPUCapacity;
static MetricCalculator  metricCalcKernelModePercentage;
static MetricCalculator  metricCalcUserModePercentage;
static MetricCalculator  metricCalcTotalCPUPercentage;
static MetricCalculator  metricCalcIdlePercentage;
static MetricCalculator  metricCalcActiveVirtualProcessors;

/* unit definitions */
static char * muNa = "n/a";
static char * muMilliSeconds = "Milliseconds";
static char * muPercent = "Percent";

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
  metricCalcDef[0].mcName="_LPARTimes";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[0].mcChangeType=MD_GAUGE;
  metricCalcDef[0].mcIsContinuous=MD_TRUE;
  metricCalcDef[0].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcLPARTimes;
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
  metricCalcDef[1].mcCalc=metricCalcKernelModeTime;
  metricCalcDef[1].mcUnits=muMilliSeconds;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="UserModeTime";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_SUMMABLE;
  metricCalcDef[2].mcDataType=MD_UINT64;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcUserModeTime;
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
  metricCalcDef[4].mcName="UnusedPartitionCPUCapacity";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_SUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[4].mcCalc=metricCalcUnusedPartitionCPUCapacity;
  metricCalcDef[4].mcUnits=muMilliSeconds;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="ExternalViewKernelModePercentage";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[5].mcDataType=MD_FLOAT32;
  metricCalcDef[5].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[5].mcCalc=metricCalcKernelModePercentage;
  metricCalcDef[5].mcUnits=muPercent;

  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="ExternalViewUserModePercentage";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_FLOAT32;
  metricCalcDef[6].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[6].mcCalc=metricCalcUserModePercentage;
  metricCalcDef[6].mcUnits=muPercent;

  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="ExternalViewTotalCPUPercentage";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_FLOAT32;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcTotalCPUPercentage;
  metricCalcDef[7].mcUnits=muPercent;

  metricCalcDef[8].mcVersion=MD_VERSION;
  metricCalcDef[8].mcName="ExternalViewIdlePercentage";
  metricCalcDef[8].mcId=mr(pluginname,metricCalcDef[8].mcName);
  metricCalcDef[8].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[8].mcChangeType=MD_GAUGE;
  metricCalcDef[8].mcIsContinuous=MD_TRUE;
  metricCalcDef[8].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[8].mcDataType=MD_FLOAT32;
  metricCalcDef[8].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[8].mcCalc=metricCalcIdlePercentage;
  metricCalcDef[8].mcUnits=muPercent;

  metricCalcDef[9].mcVersion=MD_VERSION;
  metricCalcDef[9].mcName="ActiveVirtualProcessors";
  metricCalcDef[9].mcId=mr(pluginname,metricCalcDef[9].mcName);
  metricCalcDef[9].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[9].mcChangeType=MD_GAUGE;
  metricCalcDef[9].mcIsContinuous=MD_TRUE;
  metricCalcDef[9].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[9].mcDataType=MD_FLOAT32;
  metricCalcDef[9].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[9].mcCalc=metricCalcActiveVirtualProcessors;
  metricCalcDef[9].mcUnits=muPercent;

  *mcnum=sizeof(metricCalcDef)/sizeof(MetricCalculationDefinition);
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* LPAR Times (internal only)                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcLPARTimes( MetricValue *mv,   
			    int mnum,
			    void *v, 
			    size_t vlen ) 
{
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR._LPARTimes\n", __FILE__,__LINE__);)

  if ( mv && (vlen>mv->mvDataLength)) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    return mv->mvDataLength;
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Kernel Mode Time                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcKernelModeTime( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) 
{
  unsigned long long cputime1, cputime2;
  unsigned long long mgmttime1, mgmttime2;
  unsigned long long onlinetime1, onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.KernelModeTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (mgmttime1 > mgmttime2) {
      *(unsigned long long *)v = (mgmttime1 - mgmttime2) / 1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* User Mode Time                                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUserModeTime( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) 
{
  unsigned long long cputime1, cputime2;
  unsigned long long mgmttime1, mgmttime2;
  unsigned long long onlinetime1, onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.UserModeTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (cputime1 > cputime2) {
      *(unsigned long long *)v = (cputime1 - cputime2) / 1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Total CPU Time                                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalCPUTime( MetricValue *mv,   
			       int mnum,
			       void *v, 
			       size_t vlen ) 
{
  unsigned long long cputime1, cputime2;
  unsigned long long mgmttime1, mgmttime2;
  unsigned long long onlinetime1, onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.TotalCPUTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (cputime1 + mgmttime1 > cputime2 + mgmttime2) {
      *(unsigned long long *)v = 
	((cputime1 + mgmttime1) - (cputime2 + mgmttime2)) / 1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Unused Partition CPU Capacity                                              */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUnusedPartitionCPUCapacity( MetricValue *mv,   
					     int mnum,
					     void *v, 
					     size_t vlen ) 
{
  unsigned long long cputime1, cputime2;
  unsigned long long mgmttime1, mgmttime2;
  unsigned long long onlinetime1, onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.UnusedPartitionCPUCapacity\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if ((onlinetime1-cputime1-mgmttime1) > (onlinetime2-cputime2-mgmttime2)) {
      *(unsigned long long *)v = 
	((onlinetime1-cputime1-mgmttime1) - (onlinetime2-cputime2-mgmttime2)) / 
	1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Kernel Mode Percentage                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcKernelModePercentage( MetricValue *mv,   
				       int mnum,
				       void *v, 
				       size_t vlen ) 
{
  unsigned long long cputime1,cputime2;
  unsigned long long mgmttime1,mgmttime2;
  unsigned long long onlinetime1,onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.KernelModePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (onlinetime1>onlinetime2) {
      if (mgmttime1 > mgmttime2) {
	*(float *)v = (float)(mgmttime1-mgmttime2)/(onlinetime1-onlinetime2)*100;
      } else {
	*(float *)v = 0.0;
      }
      return sizeof(float);
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* User Mode Percentage                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcUserModePercentage( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) 
{
  unsigned long long cputime1,cputime2;
  unsigned long long mgmttime1,mgmttime2;
  unsigned long long onlinetime1,onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.UserModePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (onlinetime1>onlinetime2) {
      if (cputime1>cputime2) {
	*(float *)v = (float)(cputime1-cputime2)/(onlinetime1-onlinetime2)*100;
      } else {
	*(float *)v = 0.0;
      }
      return sizeof(float);
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Total CPU Percentage                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTotalCPUPercentage( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) 
{
  unsigned long long cputime1,cputime2;
  unsigned long long mgmttime1,mgmttime2;
  unsigned long long onlinetime1,onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.TotalCPUPercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (onlinetime1>onlinetime2) {
      if (cputime1 + mgmttime1 > cputime2 + mgmttime2) {
	*(float *)v = (float)((cputime1 + mgmttime1) - (cputime2 + mgmttime2))/
	  (onlinetime1-onlinetime2)*100;
      } else {
	*(float *)v = 0.0;
      }
      return sizeof(float);
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Idle Percentage                                                            */
/* ---------------------------------------------------------------------------*/

size_t metricCalcIdlePercentage( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) 
{
  unsigned long long cputime1,cputime2;
  unsigned long long mgmttime1,mgmttime2;
  unsigned long long onlinetime1,onlinetime2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.IdlePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    if (onlinetime1>onlinetime2) {
      if ((onlinetime1-cputime1-mgmttime1) > (onlinetime2-cputime2-mgmttime2)) {
	*(float *)v = (float)((onlinetime1-cputime1-mgmttime1)-(onlinetime2-cputime2-mgmttime2))/
	  (onlinetime1-onlinetime2)*100;
      } else {
	*(float *)v = 0.0;
      }
      return sizeof(float);
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Active Virtual Processors                                                  */
/* ---------------------------------------------------------------------------*/

size_t metricCalcActiveVirtualProcessors( MetricValue *mv,   
					  int mnum,
					  void *v, 
					  size_t vlen ) 
{
  unsigned long long cputime1,cputime2;
  unsigned long long mgmttime1,mgmttime2;
  unsigned long long onlinetime1,onlinetime2;
  time_t             interval;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate LPAR.ActiveVirtualProcessors\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    sscanf(mv[0].mvData,"%llu:%llu:%llu",&cputime1,&mgmttime1,&onlinetime1);
    sscanf(mv[mnum-1].mvData,"%llu:%llu:%llu",&cputime2,&mgmttime2,&onlinetime2);
    interval = mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp;
    if ( interval > 0 && onlinetime1 > onlinetime2) {
      /* raw values are microseconds */
      *(float*) v = (float)(onlinetime1 - onlinetime2)/(interval*1000000);
      return sizeof(float);
    }
  }
  return -1;
}

