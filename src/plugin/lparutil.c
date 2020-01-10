/*
 * $Id: lparutil.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Utility Functions for CEC and LPAR Metric Plugins
 *
 */

/* ---------------------------------------------------------------------------*/

#include <stdlib.h>
#include "lparutil.h"

pthread_mutex_t  datamutex  = PTHREAD_MUTEX_INITIALIZER;
HypSystem       *systemdata = NULL;
HypSystem       *systemdata_old = NULL;

unsigned long long cpusum(int cputime, HypSystem *system)
{
  int i;
  unsigned long long sum = 0;
  
  for (i=0; i<system->hs_numcpu;i++) {
    switch (cputime) {
    case CPU_MGMTTIME:
      if (system->hs_cpu[i].hp_mgmttime != -1) {
	sum += system->hs_cpu[i].hp_mgmttime;
      }
      break;
    case CPU_ONLINETIME:
      if (system->hs_cpu[i].hp_onlinetime != -1) {
	sum += system->hs_cpu[i].hp_onlinetime;
      }
      break;
    case CPU_TIME:
      if (system->hs_cpu[i].hp_cputime != -1) {
	sum += system->hs_cpu[i].hp_cputime;
      }
      break;
    default:
      break;
    }
  }
  return sum;
}

void refreshSystemData()
{
  time_t now;

  if (pthread_mutex_lock(&datamutex) == 0) {
    now = time(NULL);
    if (systemdata == NULL) {
      systemdata=get_hypervisor_info(get_hypervisor_rootpath());
    } else if (abs(now-systemdata->hs_timestamp) > 2 ) {
      if (systemdata_old) {
	release_hypervisor_info(systemdata_old);
      }
      systemdata_old = systemdata;
      systemdata=get_hypervisor_info(get_hypervisor_rootpath());
    }
    pthread_mutex_unlock(&datamutex);
  }
}
