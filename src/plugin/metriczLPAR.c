/*
 * $Id: metriczLPAR.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Metrics Gatherer Plugin of the following CEC and LPAR Metrics :
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
#include <time.h>
#include <unistd.h>
#include <commutil.h>
#include "lparutil.h"

/* ---------------------------------------------------------------------------*/

static MetricDefinition metricDef[1];

static MetricRetriever  metricRetrLPAR;

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
  metricDef[0].mdName="_LPARTimes";
  metricDef[0].mdReposPluginName="librepositoryzLPAR.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrLPAR;
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

int metricRetrLPAR( int mid, MetricReturner mret ) 
{
  MetricValue *mv;
  int          num=0;
  char        *mbufform = "%llu:%llu:%llu";
  int          mbuflen = strlen(mbufform) + 3 * strlen("18446744073709551615ULL") + 1;
  int          i;

#ifdef DEBUG  
  fprintf(stderr,"--- %s(%i) : retrieving %d\n",
	  __FILE__,__LINE__,mid);
#endif

  refreshSystemData();
  if (pthread_mutex_lock(&datamutex) == 0) {
    if (systemdata) {
      for (i=0; i < systemdata->hs_numsys; i++) {
	mv = calloc(1, sizeof(MetricValue) + 
		    mbuflen +
		    (strlen(systemdata->hs_sys[i].hs_id)+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = systemdata->hs_timestamp;
	  mv->mvDataType = MD_STRING;
	  mv->mvDataLength = mbuflen;
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  snprintf(mv->mvData,mbuflen,mbufform,
		   cpusum(CPU_TIME,systemdata->hs_sys+i),
		   cpusum(CPU_MGMTTIME,systemdata->hs_sys+i),
		   cpusum(CPU_ONLINETIME,systemdata->hs_sys+i));
	  mv->mvResource = mv->mvData + strlen(mv->mvData) + 1;
	  strcpy(mv->mvResource,systemdata->hs_sys[i].hs_id);
	  mret(mv);
	  num += 1;
	} 
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
