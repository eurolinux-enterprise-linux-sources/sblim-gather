/*
 * $Id: repositoryzCEC.c,v 1.4 2012/07/31 23:26:46 tyreld Exp $
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
 * Metrics Repository Plugin of the following CEC Metrics :
 *  IBM_zCEC.KernelModeTime
 *  IBM_zCEC.UserModeTime
 *  IBM_zCEC.TotalCPUTime
 *  IBM_zCEC.UnusedGlobalCPUCapacity
 *  IBM_zCEC.ExternalViewKernelModePercentage
 *  IBM_zCEC.ExternalViewUserModePercentage
 *  IBM_zCEC.ExternalViewTotalCPUPercentage
 *
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
static MetricCalculationDefinition metricCalcDef[8];

/* metric calculator callbacks */
static MetricCalculator  metricCalc_CECTimes;
static MetricCalculator  metricCalcCECKernelModeTime;
static MetricCalculator  metricCalcCECUserModeTime;
static MetricCalculator  metricCalcCECTotalCPUTime;
static MetricCalculator  metricCalcCECUnusedGlobalCPUCapacity;
static MetricCalculator  metricCalcCECExternalViewKernelModePercentage;
static MetricCalculator  metricCalcCECExternalViewUserModePercentage;
static MetricCalculator  metricCalcCECExternalViewTotalCPUPercentage;

/* unit definitions */
static char * muNA = "n/a";
static char * muMilliSeconds = "Milliseconds";
static char * muPercent = "Percent";

/* helper */
static int parseCECTimes(const char * s, unsigned long long * kerneltime, 	 
			 unsigned long long * usertime, 
			 unsigned long long * kerneltime_adj, 
			 unsigned long long * usertime_adj, 
			 int * numcpus);

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
  metricCalcDef[0].mcName="_CECTimes";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[0].mcChangeType=MD_GAUGE;
  metricCalcDef[0].mcIsContinuous=MD_TRUE;
  metricCalcDef[0].mcCalculable=MD_SUMMABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalc_CECTimes;
  metricCalcDef[0].mcUnits=muNA;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="KernelModeTime";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_SUMMABLE;
  metricCalcDef[1].mcDataType=MD_UINT64;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcCECKernelModeTime;
  metricCalcDef[1].mcUnits=muMilliSeconds;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="UserModeTime";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_SUMMABLE;
  metricCalcDef[2].mcDataType=MD_UINT64;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcCECUserModeTime;
  metricCalcDef[2].mcUnits=muMilliSeconds;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="TotalCPUTime";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_SUMMABLE;
  metricCalcDef[3].mcDataType=MD_UINT64;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcCECTotalCPUTime;
  metricCalcDef[3].mcUnits=muMilliSeconds;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="UnusedGlobalCPUCapacity";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_SUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[4].mcCalc=metricCalcCECUnusedGlobalCPUCapacity;
  metricCalcDef[4].mcUnits=muMilliSeconds;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="ExternalViewKernelModePercentage";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[5].mcDataType=MD_FLOAT32;
  metricCalcDef[5].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[5].mcCalc=metricCalcCECExternalViewKernelModePercentage;
  metricCalcDef[5].mcUnits=muPercent;
 
  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="ExternalViewUserModePercentage";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_FLOAT32;
  metricCalcDef[6].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[6].mcCalc=metricCalcCECExternalViewUserModePercentage;
  metricCalcDef[6].mcUnits=muPercent;
 
  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="ExternalViewTotalCPUPercentage";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_FLOAT32;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcCECExternalViewTotalCPUPercentage;
  metricCalcDef[7].mcUnits=muPercent;
 
  *mcnum=sizeof(metricCalcDef)/sizeof(MetricCalculationDefinition);
  *mc=metricCalcDef;
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* _CECTimes (compound raw metric)                                            */
/* ---------------------------------------------------------------------------*/

size_t metricCalc_CECTimes( MetricValue *mv,   
			    int mnum,
			    void *v, 
			    size_t vlen ) 
{
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC._CECTimes\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=strlen(mv->mvData))) {
    strcpy(v,mv->mvData);
    return strlen(mv->mvData);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Kernel Mode Time                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECKernelModeTime( MetricValue *mv,   
				    int mnum,
				    void *v, 
				    size_t vlen ) 
{
  unsigned long long kmt1, kmt2, kmt_adj, not_used1;
  int not_used2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.KernelModeTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    parseCECTimes(mv[0].mvData,&kmt1,&not_used1,&kmt_adj,&not_used1,&not_used2);
    parseCECTimes(mv[mnum-1].mvData,&kmt2,&not_used1,&not_used1,&not_used1,&not_used2);
    if (kmt1 + kmt_adj > kmt2) {
      *(unsigned long long *)v = (kmt1 - kmt2 + kmt_adj) / 1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* User Mode Time                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECUserModeTime( MetricValue *mv,   
				  int mnum,
				  void *v, 
				  size_t vlen ) 
{
  unsigned long long umt1, umt2, umt_adj, not_used1;
  int not_used2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.UserModeTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    parseCECTimes(mv[0].mvData,&not_used1,&umt1,&not_used1,&umt_adj,&not_used2);
    parseCECTimes(mv[mnum-1].mvData,&not_used1,&umt2,&not_used1,&not_used1,&not_used2);
    if (umt1 + umt_adj > umt2) {
      *(unsigned long long *)v = (umt1 - umt2 + umt_adj) / 1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Total CPU Time                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECTotalCPUTime( MetricValue *mv,   
				  int mnum,
				  void *v, 
				  size_t vlen ) 
{
  unsigned long long umt1, umt2, umt_adj, not_used1;
  unsigned long long kmt1, kmt2, kmt_adj;
  int not_used2;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.TotalCPUTime\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    parseCECTimes(mv[0].mvData,&kmt1,&umt1,&kmt_adj,&umt_adj,&not_used2);
    parseCECTimes(mv[mnum-1].mvData,&kmt2,&umt2,&not_used1,&not_used1,&not_used2);
    if (umt1 + kmt1 + umt_adj + kmt_adj > umt2 + kmt2) {
      *(unsigned long long *)v = ((kmt1 + umt1) - (kmt2 + umt2) + kmt_adj + umt_adj)/1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* Unused Global CPU Capacity                                                 */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECUnusedGlobalCPUCapacity( MetricValue *mv,   
					     int mnum,
					     void *v, 
					     size_t vlen ) 
{
  unsigned long long umt1, umt2, umt_adj;
  unsigned long long kmt1, kmt2, kmt_adj, not_used1;
  int                num_cpus, not_used2;
  time_t             interval;

  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.UnusedGlobalCPUCapacity\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(unsigned long long))) {
    parseCECTimes(mv[0].mvData,&kmt1,&umt1,&kmt_adj,&umt_adj,&num_cpus);
    parseCECTimes(mv[mnum-1].mvData,&kmt2,&umt2,&not_used1,&not_used1,&not_used2);
    interval = mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp;
    if ( interval > 0 && (kmt1 + umt1 + kmt_adj + umt_adj > kmt2 + umt2)) {
      *(unsigned long long *)v = interval*1000*num_cpus - ((kmt1 + umt1) - (kmt2 + umt2) + kmt_adj + umt_adj)/1000;
    } else {
      *(unsigned long long *)v = 0ULL;
    }
    return sizeof(unsigned long long);
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* ExternalViewKernelModePercentage                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECExternalViewKernelModePercentage( MetricValue *mv,   
						      int mnum,
						      void *v, 
						      size_t vlen ) 
{
  unsigned long long kmt1;
  unsigned long long kmt2;
  unsigned long long kmt_adj, not_used1;
  int                num_cpus, not_used2;
  time_t             interval;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.ExternalViewKernelModePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    parseCECTimes(mv[0].mvData,&kmt1,&not_used1,&kmt_adj,&not_used1,&num_cpus);
    parseCECTimes(mv[mnum-1].mvData,&kmt2,&not_used1,&not_used1,&not_used1,&not_used2);
    interval = mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp;
    if ( interval > 0 && (kmt1 + kmt_adj > kmt2)) {
      /* raw values are microseconds */
      *(float*) v = (float)(kmt1 - kmt2 + kmt_adj)/(interval*10000*num_cpus);
      return sizeof(float);
    } else {
      *(float*) v = 0;
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* ExternalViewUserModePercentage                                             */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECExternalViewUserModePercentage( MetricValue *mv,   
						    int mnum,
						    void *v, 
						    size_t vlen ) 
{
  unsigned long long umt1;
  unsigned long long umt2;
  unsigned long long umt_adj, not_used1;
  int                num_cpus, not_used2;
  time_t             interval;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.ExternalViewUserModePercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    parseCECTimes(mv[0].mvData,&not_used1,&umt1,&not_used1,&umt_adj,&num_cpus);
    parseCECTimes(mv[mnum-1].mvData,&not_used1,&umt2,&not_used1,&not_used1,&not_used2);
    interval = mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp;
    if ( interval > 0 && (umt1 + umt_adj > umt2)) {
      /* raw values are microseconds */
      *(float*) v = (float)(umt1 - umt2 + umt_adj)/(interval*10000*num_cpus);
      return sizeof(float);
    } else {
      *(float*) v = 0;
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------------*/
/* ExternalViewTotalCPUPercentage                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCECExternalViewTotalCPUPercentage( MetricValue *mv,   
						    int mnum,
						    void *v, 
						    size_t vlen ) 
{
  unsigned long long kmt1, umt1;
  unsigned long long kmt2, umt2;
  unsigned long long kmt_adj, umt_adj, not_used1;
  int                num_cpus, not_used2;
  time_t             interval;
  DBGONLY(fprintf(stderr,"--- %s(%i) : Calculate CEC.ExternalViewTotalCPUPercentage\n", __FILE__,__LINE__);)

  if ( mv && (vlen>=sizeof(float))) {
    parseCECTimes(mv[0].mvData,&kmt1,&umt1,&kmt_adj,&umt_adj,&num_cpus);
    parseCECTimes(mv[mnum-1].mvData,&kmt2,&umt2,&not_used1,&not_used1,&not_used2);
    interval = mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp;
    if ( interval > 0 && (kmt1 + umt1 + kmt_adj + umt_adj > kmt2 + umt2)) {
      /* raw values are microseconds */
      *(float*) v = (float)((umt1 + kmt1) - (umt2 + kmt2) + umt_adj + kmt_adj)/(interval*10000*num_cpus);
      return sizeof(float);
    } else {
      *(float*) v = 0;
    }
  }
  return -1;
}

static int parseCECTimes(const char * s, unsigned long long * kerneltime, 	 
			 unsigned long long * usertime, 
			 unsigned long long * kerneltime_adj, 
			 unsigned long long * usertime_adj, 
			 int * numcpus)
{
  /* scan the _CECTimes metric in a backward-compatible way */
  int num = sscanf(s,"%llu:%d:%llu:%llu:%llu",kerneltime,numcpus,usertime,kerneltime_adj,usertime_adj);
  if (num < 3) {
    return -1;
  } else {
    if (num < 5) {
      usertime_adj=0ULL;
    }
    if (num < 4) {
      kerneltime_adj=0ULL;
    }
    return 0;
  }
}
