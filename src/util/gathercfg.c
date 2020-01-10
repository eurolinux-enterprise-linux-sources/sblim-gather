/*
 * $Id: gathercfg.c,v 1.7 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Gatherd Configuration
 *
 */

#include "gathercfg.h"
#include "mcfg.h"

#ifndef GATHER_CONFDIR
#define GATHER_CONFDIR "/etc"
#endif

static int gathercfg_handle;

int gathercfg_init()
{
  const char * allowed[] = {
    "RepositoryHost",
    "RepositoryPort",
    "TraceLevel",
    "TraceFile",
    "TraceComponents",
    "PluginDirectory",
    "AutoLoad",
    "SampleInterval",
    "Synchronization",
    NULL
  };
  gathercfg_handle=set_configfile(GATHER_CONFDIR "/gatherd.conf",allowed);
  return gathercfg_handle > 0 ? 0 : 1;
}

int gathercfg_getitem(const char * key, char * value, size_t maxlen)
{
  return get_configitem(gathercfg_handle, key, value, maxlen);
}
