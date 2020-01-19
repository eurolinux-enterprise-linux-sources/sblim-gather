/*
 * $Id: metriczCEC.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Metrics Gatherer Plugin of the following CEC Metrics :
 *  IBM_zCEC.KernelModeTime
 *  IBM_zCEC.UserModeTime
 *  IBM_zCEC.TotalCPUTime
 *  IBM_zCEC.UnusedGlobalCPUCapacity
 *  IBM_zCEC.ExternalViewKernelModePercentage
 *  IBM_zCEC.ExternalViewUserModePercentage
 *  IBM_zCEC.ExternalViewTotalCPUPercentage
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
#include "lparutil.h"

/* ---------------------------------------------------------------------------*/

static MetricDefinition metricDef[1];

static MetricRetriever  metricRetrCEC;

/* ---------------------------------------------------------------------------*/


int _DefinedMetrics( MetricRegisterId *mr,
		     const char * pluginname,
		     size_t *mdnum,
		     MetricDefinition **md ) 
{

#ifdef DEBUG  
  fprintf(stderr,"--- %s(%i) : Retrieving metric definitions\n",
	  __FILE__,__LINE__);
#endif
    
  if (mr==NULL||mdnum==NULL||md==NULL) {
#ifdef DEBUG  
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
#endif
    return -1;
  }
  
  metricDef[0].mdVersion=MD_VERSION;
  metricDef[0].mdName="_CECTimes";
  metricDef[0].mdReposPluginName="librepositoryzCEC.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrCEC;
  metricDef[0].mdeal=free;

  *mdnum=sizeof(metricDef)/sizeof(MetricDefinition);
  *md=metricDef;
  return 0;
}

int _StartStopMetrics (int starting) 
{

  if( starting ) {
#ifdef DEBUG  
    fprintf(stderr,"--- %s(%i) : Metric processing starting\n",
	    __FILE__,__LINE__);
#endif
    /* nothing to do */
    refreshSystemData();
    if (systemdata == NULL) {
      return 0;
      /* should be: return -1; */
    }
  } else {
#ifdef DEBUG  
    fprintf(stderr,"--- %s(%i) : Metric processing stopping\n",
	    __FILE__,__LINE__);
#endif
    if (pthread_mutex_lock(&datamutex) == 0) {
      if (systemdata) {
	release_hypervisor_info(systemdata);
	systemdata = NULL;
      }
      pthread_mutex_unlock(&datamutex);
    }
  }
  
  return 0;
}

int metricRetrCEC( int mid, MetricReturner mret ) 
{
  MetricValue       *mv;
  int                num=0;
  size_t             maxvallen = strlen("18446744073709551615ULL:4294967296:18446744073709551615ULL:18446744073709551615ULL:18446744073709551615ULL") + 1;
  unsigned long long utime, ktime, utime_a, ktime_a;
  unsigned long long usertime = 0ULL;  
  unsigned long long kerneltime = 0ULL;  
  unsigned long long usertime_adj = 0ULL;  
  unsigned long long kerneltime_adj = 0ULL;  
  int i;

  /* Format: <LPAR mgmt time>:<number physical CPUs>:< user time>:<LPAR time adjustment>:<user time adjustment> */

#ifdef DEBUG  
  fprintf(stderr,"--- %s(%i) : retrieving %d\n",
	  __FILE__,__LINE__,mid);
#endif

  refreshSystemData();
  if (pthread_mutex_lock(&datamutex) == 0) {
    if (systemdata) {
      mv = calloc(1, sizeof(MetricValue) + 
		  maxvallen +
		  (strlen(systemdata->hs_id)+1) );
      if (mv) {
	/* compute totals */
	for (i=0;i<systemdata->hs_numsys;i++) {
	  utime = cpusum(CPU_TIME,systemdata->hs_sys+i);
	  ktime = cpusum(CPU_MGMTTIME,systemdata->hs_sys+i);
	  usertime += utime;
	  kerneltime += ktime;
	  if (systemdata_old && 
	      systemdata_old->hs_numsys == systemdata->hs_numsys &&
	      strcmp(systemdata->hs_sys[i].hs_id,systemdata_old->hs_sys[i].hs_id) == 0) {
	    utime_a = cpusum(CPU_TIME,systemdata_old->hs_sys+i);
	    ktime_a = cpusum(CPU_MGMTTIME,systemdata_old->hs_sys+i);
	    /* check if an LPAR has been deactivated since the last sample and add adjustment ticks if necessary */
	    if (utime_a != (unsigned long long) -1LL &&
                (utime == (unsigned long long) -1LL || utime_a > utime)) {
	      usertime_adj += (utime_a - utime);
	    }
	    if (ktime_a != (unsigned long long) -1LL &&
                (ktime == (unsigned long long) -1LL || ktime_a > ktime)) {
	      kerneltime_adj += (ktime_a - ktime);
	    }
	  }
	}
	mv->mvId = mid;
	mv->mvTimeStamp = systemdata->hs_timestamp;
	mv->mvDataType = MD_STRING;
	mv->mvData = (char*)mv + sizeof(MetricValue);
	snprintf(mv->mvData,maxvallen,"%llu:%d:%llu:%llu:%llu",
		 cpusum(CPU_MGMTTIME,systemdata) + kerneltime,
		 systemdata->hs_numcpu,usertime, kerneltime_adj, usertime_adj);
	mv->mvDataLength = strlen(mv->mvData) + 1;
	mv->mvResource = mv->mvData + mv->mvDataLength;
	strcpy(mv->mvResource,systemdata->hs_id);
	mret(mv);
	num = 1;
      }  
    }
    pthread_mutex_unlock(&datamutex);
  }
#ifdef DEBUG  
  fprintf(stderr,"--- %s(%i) : returning %d\n",
	  __FILE__,__LINE__,num);
#endif
  return num;
}

