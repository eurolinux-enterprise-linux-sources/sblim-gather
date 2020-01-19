/*
 * $Id: gatherc.h,v 1.10 2009/06/19 20:04:54 tyreld Exp $
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
 * Description:  Gatherer Communications Definition
 *
 */

#ifndef GATHERC_H
#define GATHERC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define GATHERMC_REQ       1
#define GATHERMC_RESP      2 
#define GATHERMC_REQMORE   3
#define GATHERMC_RESPMORE  4 

#define GCMD_INIT          1
#define GCMD_TERM          2
#define GCMD_START         3
#define GCMD_STOP          4
#define GCMD_STATUS        5
#define GCMD_ADDPLUGIN     6
#define GCMD_REMPLUGIN     7
#define GCMD_GETVALUE      8
#define GCMD_LISTPLUGIN    9
#define GCMD_SETVALUE      10
#define GCMD_GETTOKEN      11
#define GCMD_SUBSCRIBE     12
#define GCMD_LISTRESOURCES 13
#define GCMD_UNSUBSCRIBE   14
#define GCMD_GETFILTERED   15
#define GCMD_SETFILTER     16
#define GCMD_GETFILTER     17

#define GCMD_QUIT          99

#define GATHERBUFLEN       1000
#define GATHERVALBUFLEN    10000

#define GATHER_COMMID "gather"
#define REPOS_COMMID  "repos"


typedef struct _GATHERCOMM {
  short    gc_cmd;
  short    gc_result;
  unsigned gc_datalen;
} GATHERCOMM;

#ifdef __cplusplus
}
#endif

#endif
