/*
 * $Id: mreg.c,v 1.2 2009/05/20 19:39:55 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Metric Registry;
 * Central Place where metric definitions are registered and can
 * be looked up.
 */

#include "mreg.h"
#include "mrwlock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>


/* Metric Entry Record for Registry */
typedef struct _MetricEntry {
  int               meId;
  char             *meName;
  MetricDefinition *meDef;
} MetricEntry;

/* operational definitions */
static const size_t INCREASE_BY = 100;
static const int ID_OFFSET = 111;

#define INDEX_FOR_ID(id) ((id)-ID_OFFSET)
#define ID_FOR_INDEX(idx) ((idx)+ID_OFFSET)

/* Registry control data */
static MetricEntry *MR_Entries = NULL;
static size_t       MR_NumEntries = 0;
static size_t       MR_MaxEntries = 0;

/* Utility Functions */
static void Grow();
static int NextIndex();

/* Semaphore Definition */

MRWLOCK_DEFINE(MRegLock);

/*
 * Initialize the Registry - allocates initial slots.
 */
void MPR_InitRegistry()
{
  MWriteLock(&MRegLock);
  Grow();
  MWriteUnlock(&MRegLock);
}

/*
 * Flushes Registry 
 */
void MPR_FinishRegistry()
{
  size_t i;
  MWriteLock(&MRegLock);
  for (i=0;i<MR_NumEntries;i++)
    if (MR_Entries[i].meName)
      free(MR_Entries[i].meName);
  MR_NumEntries = MR_MaxEntries = 0;
  free(MR_Entries);
  MR_Entries = NULL;
  MWriteUnlock(&MRegLock);
}

/*
 * Returns a unique id for a given name. A slot is allocated if
 * necessary.
 */
int MPR_IdForString(const char *pluginname, const char * name)
{
  size_t i;
  int id;
  char fullname[300];
  sprintf(fullname,"%s:%s",pluginname,name);
  MReadLock(&MRegLock);
  for (i=0, id=-1; i<MR_NumEntries;i++) {    
    if (MR_Entries[i].meName && strcmp(MR_Entries[i].meName,fullname)==0) {
      id=MR_Entries[i].meId;
      break;
    }
  }
  MReadUnlock(&MRegLock);
  if (id==-1) {
    /* not found - must create slot */
    MWriteLock(&MRegLock);
    i = NextIndex();
    id = MR_Entries[i].meId = ID_FOR_INDEX(i);
    MR_Entries[i].meName = strdup(fullname);
    MR_Entries[i].meDef = NULL;
    MWriteUnlock(&MRegLock);
  }
  return id;
}

/*
 * Adds or updates the registry with a metric definition.
 * An existing slot will be reused.
 */
int  MPR_UpdateMetric(const char * pluginname, MetricDefinition *metric)
{
  int id;
  id = MPR_IdForString(pluginname,metric->mdName);
  if (id!=-1) {
    MWriteLock(&MRegLock);
    MR_Entries[INDEX_FOR_ID(id)].meDef = metric;
    MWriteUnlock(&MRegLock);
    return 0;
  }
  return -1;
}

/*
 * Retrieves a metric definition.
 * This function SHOULD perform well!
 */
MetricDefinition* MPR_GetMetric(int id)
{
  /* Lock doesn't make sense here - data is always consistent */
  if (INDEX_FOR_ID(id) < MR_NumEntries)
    return MR_Entries[INDEX_FOR_ID(id)].meDef;
  else
    return NULL;
}

/*
 * Removes a metric definition from the registry.
 * The slot and id are lost and cannot be reused.
 */
void MPR_RemoveMetric(int id)
{
  MWriteLock(&MRegLock);
  if (INDEX_FOR_ID(id)<MR_NumEntries && MR_Entries[INDEX_FOR_ID(id)].meDef) {
    free(MR_Entries[INDEX_FOR_ID(id)].meName);
    MR_Entries[INDEX_FOR_ID(id)].meName=NULL;
    MR_Entries[INDEX_FOR_ID(id)].meDef=NULL;
    MR_Entries[INDEX_FOR_ID(id)].meId=0;
  }
  MWriteUnlock(&MRegLock);
}

static void Grow()
{
  if (MR_MaxEntries == MR_NumEntries) {
    MR_MaxEntries += INCREASE_BY;
    MR_Entries = realloc(MR_Entries,MR_MaxEntries*sizeof(MetricEntry));
    memset(MR_Entries+MR_MaxEntries-INCREASE_BY,0,INCREASE_BY);
  }
}

static int NextIndex()
{
  if (MR_MaxEntries == MR_NumEntries)
    Grow();
  return MR_NumEntries++;
}
