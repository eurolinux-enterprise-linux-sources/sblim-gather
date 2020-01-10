/*
 * $Id: rreg.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Repository Metric Registry;
 * Central Place where repository metric definitions are registered and can
 * be looked up.
 */

#include "rreg.h"
#include "mrwlock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>


/* Metric Entry Record for Registry */
typedef struct _MetricCalcEntry {
  int                          mceId;
  char                        *mceName;
  MetricCalculationDefinition *mceDef;
} MetricCalcEntry;

/* operational definitions */
static const size_t INCREASE_BY = 100;
static const int ID_OFFSET = 111;

#define INDEX_FOR_ID(id) ((id)-ID_OFFSET)
#define ID_FOR_INDEX(idx) ((idx)+ID_OFFSET)

/* Registry control data */
static MetricCalcEntry *PR_Entries = NULL;
static size_t           PR_NumEntries = 0;
static size_t           PR_MaxEntries = 0;

/* Utility Functions */
static void Grow();
static int NextIndex();

/* Semaphore Definition */

MRWLOCK_DEFINE(RregLock);

/*
 * Initialize the Registry - allocates initial slots.
 */
void RPR_InitRegistry()
{
  MWriteLock(&RregLock);
  Grow();
  MWriteUnlock(&RregLock);
}

/*
 * Flushes Registry 
 */
void RPR_FinishRegistry()
{
  int i;
  MWriteLock(&RregLock);
  for (i=0;i<PR_NumEntries;i++)
    if (PR_Entries[i].mceName)
      free(PR_Entries[i].mceName);
  PR_NumEntries = PR_MaxEntries = 0;
  free(PR_Entries);
  PR_Entries = NULL;
  MWriteUnlock(&RregLock);
}

/*
 * Returns a unique id for a given name. A slot is allocated if
 * necessary.
 */
int RPR_IdForString(const char *pluginname, const char * name)
{
  int i,id;
  size_t len;
  char fullname[300];
  /* this is called quite often - make it efficient */
  len = strlen(pluginname);
  memcpy(fullname,pluginname,len);
  fullname[len]=':';
  strcpy(fullname+len+1,name);
  MReadLock(&RregLock);
  for (i=0, id=-1; i<PR_NumEntries;i++) {    
    if (PR_Entries[i].mceName && strcmp(PR_Entries[i].mceName,fullname)==0) {
      id=PR_Entries[i].mceId;
      break;
    }
  }
  MReadUnlock(&RregLock);
  if (id==-1) {
    /* not found - must create slot */
    MWriteLock(&RregLock);
    i = NextIndex();
    id = PR_Entries[i].mceId = ID_FOR_INDEX(i);
    PR_Entries[i].mceName = strdup(fullname);
    PR_Entries[i].mceDef = NULL;
    MWriteUnlock(&RregLock);
  }
  return id;
}

/*
 * Adds or updates the registry with a metric definition.
 * An existing slot will be reused.
 */
int  RPR_UpdateMetric(const char * pluginname, MetricCalculationDefinition *metric)
{
  int id;
  id = RPR_IdForString(pluginname,metric->mcName);
  if (id!=-1) {
    MWriteLock(&RregLock);
    PR_Entries[INDEX_FOR_ID(id)].mceDef = metric;
    MWriteUnlock(&RregLock);
    return 0;
  }
  return -1;
}

/*
 * Retrieves a metric definition.
 * This function SHOULD perform well!
 */
MetricCalculationDefinition* RPR_GetMetric(int id)
{
  /* Lock doesn't make sense here - data is always consistent */
  if (INDEX_FOR_ID(id) < PR_NumEntries)
    return PR_Entries[INDEX_FOR_ID(id)].mceDef;
  else
    return NULL;
}

/*
 * Removes a metric definition from the registry.
 * The slot and id are lost and cannot be reused.
 */
void RPR_RemoveMetric(int id)
{
  MWriteLock(&RregLock);
  if (INDEX_FOR_ID(id)<PR_NumEntries && PR_Entries[INDEX_FOR_ID(id)].mceDef) {
    free(PR_Entries[INDEX_FOR_ID(id)].mceName);
    PR_Entries[INDEX_FOR_ID(id)].mceName=NULL;
    PR_Entries[INDEX_FOR_ID(id)].mceDef=NULL;
    PR_Entries[INDEX_FOR_ID(id)].mceId=0;
  }
  MWriteUnlock(&RregLock);
}

static void Grow()
{
  if (PR_MaxEntries == PR_NumEntries) {
    PR_MaxEntries += INCREASE_BY;
    PR_Entries = realloc(PR_Entries,PR_MaxEntries*sizeof(MetricCalcEntry));
    memset(PR_Entries+PR_MaxEntries-INCREASE_BY,0,INCREASE_BY);
  }
}

static int NextIndex()
{
  if (PR_MaxEntries == PR_NumEntries)
    Grow();
  return PR_NumEntries++;
}
