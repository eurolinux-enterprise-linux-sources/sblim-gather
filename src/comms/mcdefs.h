/*
 * $Id: mcdefs.h,v 1.7 2009/05/20 19:39:56 tyreld Exp $ 
 *
 * (C) Copyright IBM Corp. 2003, 2004, 2009
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
 * Description: Communication API Defintions
 */

#ifndef MCDEFS_H
#define MCDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <merrdefs.h>

#define MC_ERR_BASE          MERRNO_BASE_COMM
#define MC_ERR_NOCONNECT     (MC_ERR_BASE+ 1)
#define MC_ERR_BADINIT       (MC_ERR_BASE+ 2)
#define MC_ERR_INVHANDLE     (MC_ERR_BASE+ 3)
#define MC_ERR_IOFAIL        (MC_ERR_BASE+ 4)
#define MC_ERR_INVPARAM      (MC_ERR_BASE+ 5)
#define MC_ERR_OVERFLOW      (MC_ERR_BASE+ 6)


#ifndef GATHER_RUNDIR
#define GATHER_RUNDIR "/var/run/gather"
#endif
  
#define MC_SOCKET    GATHER_RUNDIR "/.%s-socket"
#define MC_LOCKFILE  GATHER_RUNDIR "/.%s-lockfile"
#define MC_IPCFILE   GATHER_RUNDIR "/.%s-ipc"
#define MC_IPCPROJ 100

struct _MC_REQHDR
{
  int      mc_handle;
  unsigned mc_type;

};

typedef struct _MC_REQHDR MC_REQHDR;

typedef unsigned RC_SIZE_T;

#ifdef __cplusplus
}
#endif

#endif
