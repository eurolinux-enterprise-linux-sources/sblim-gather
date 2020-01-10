/*
 * $Id: hypfs.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Hypervisor FS Support Declarations
 *
 */

#ifndef HYPFS_H
#define HYPFS_H

#include <time.h>

#define HYPFS_MAXNAME 260

#define HYPFS_FSTYPE          "hypfs"
#ifndef HYPFS_CANONICAL_MOUNT
#define HYPFS_CANONICAL_MOUNT "/sys/hypervisor"
#endif

#define HYPFS_PHYSICAL 1
#define HYPFS_VIRTUAL  2


typedef struct _HypProcessor HypProcessor;
typedef struct _HypSystem HypSystem;

struct _HypProcessor {
  int                hp_vtype;     /* type: physical,virtual */
  char               hp_id[HYPFS_MAXNAME+1];  /* id: number, name */
  char               hp_info[HYPFS_MAXNAME+1];  /* more info: type, features... */
  unsigned long long hp_mgmttime;
  unsigned long long hp_cputime;
  unsigned long long hp_onlinetime;
};

struct _HypSystem {
  int                   hs_vtype;  /* type: physical,virtual */
  char                  hs_id[HYPFS_MAXNAME+1];  /* id: platform, lpar name,...*/
  time_t                hs_timestamp; /* point in time of measurement */
  int                   hs_numsys; /* number of contained systems */
  int                   hs_numcpu; /* number of contained processors */
  HypSystem     *hs_sys;    /* contained systems */
  HypProcessor  *hs_cpu;    /* contained processors */
};

HypSystem * get_hypervisor_info(const char *hypfspath);
void release_hypervisor_info(HypSystem * hypsys);

/* the following function is not threadsafe */
const char * get_hypervisor_rootpath(void);

#endif
