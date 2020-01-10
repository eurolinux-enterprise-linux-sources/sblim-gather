/*
 * $Id: lparutil.h,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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

#ifndef LPARUTIL_H
#define LPARUTIL_H

#include <pthread.h>
#include "hypfs.h"

extern pthread_mutex_t  datamutex;
extern HypSystem       *systemdata;
extern HypSystem       *systemdata_old;

#define CPU_MGMTTIME   0
#define CPU_ONLINETIME 1
#define CPU_TIME       2

unsigned long long cpusum(int cputime, HypSystem *system);
void refreshSystemData();

#endif
