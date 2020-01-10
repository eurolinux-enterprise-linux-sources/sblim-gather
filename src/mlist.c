/*
 * $Id: mlist.c,v 1.3 2009/05/20 19:39:55 tyreld Exp $
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
 * Description: List of Metric Blocks
 *
 * This is the central data structure for a metrics data processor.
 * Metric Blocks can be added to the list at any time between the Init
 * and Finish calls.
 * The metric blocks are sorted in the order of their next due time
 * and can be selected for processing using the SelectNextDue call.
 */

#include "mlist.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct _MutexML_Head {
  MetricBlock    *first;
  pthread_mutex_t mutex;
  int             mlsynclevel;
} MutexML_Head;

/*
 * Create a list header.
 */
ML_Head ML_Init(int synclevel)
{
  MutexML_Head *mh = malloc(sizeof(MutexML_Head));
  
  mh->first = NULL;
  pthread_mutex_init(&(mh->mutex),NULL); /* never fails */
  mh->mlsynclevel=synclevel;
  return (ML_Head)mh;
}

/*
 * Delete all metric blocks from the list and then the list itself.
 */
int ML_Finish(ML_Head mlhead)
{
  MetricBlock *mcursor;
  MutexML_Head *mh = (MutexML_Head*) mlhead; 
  
  if (pthread_mutex_lock(&(mh->mutex)))
    return -1; /* the mutex seems to be invalid - don't continue */
  while ((mcursor=mh->first)) {
    mh->first = mcursor->nextMetric;
    mcursor->metricId=-1;
    mcursor->nextMetric=NULL;
    free(mcursor);
  }
  pthread_mutex_unlock(&(mh->mutex));
  pthread_mutex_destroy(&(mh->mutex));
  mh->first=NULL;
  free(mh);
  return 0;
}

/*
 * Reset all metric blocks to READY state to resume scheduling 
 */
int ML_Reset(ML_Head mlhead)
{
  MetricBlock *mcursor;
  MutexML_Head *mh = (MutexML_Head*) mlhead; 
  time_t         t = time(NULL);
  
  if (pthread_mutex_lock(&(mh->mutex)))
    return -1; /* the mutex seems to be invalid - don't continue */
  for(mcursor=mh->first;mcursor;mcursor=mcursor->nextMetric) {
    mcursor->processingState=MSTATE_READY;
    if (mh->mlsynclevel == 1) {
      /* synchronize with wallclock */
      mcursor->nextSampleDue = t - t%mcursor->sampleInterval + mcursor->sampleInterval;
    } else {
      /* schedule asap */
      mcursor->nextSampleDue = 0;
    }
  }
  pthread_mutex_unlock(&(mh->mutex));
  return 0;
}

MetricBlock * ML_SelectNextDue(ML_Head mlhead, time_t acttime, 
			       time_t *nexttime, int reserve)
{
  MetricBlock *mcursor;
  MutexML_Head *mh = (MutexML_Head*) mlhead;

  if (!nexttime)
    return NULL;
  *nexttime = -1;
  if (pthread_mutex_lock(&(mh->mutex)))
    return NULL; /* quit if mutex not available */
  for(mcursor=mh->first;mcursor;mcursor=mcursor->nextMetric) {
    if (mcursor->nextSampleDue > acttime ) {
      /* the cursor is past the actual time */
      if (mcursor->processingState==MSTATE_READY) {
	/* found one that can be scheduled for later execution */
	*nexttime=mcursor->nextSampleDue;
	if (reserve) mcursor->processingState=MSTATE_RESERVED;
	mcursor=NULL;
	break;
      }
    } else if (mcursor->processingState==MSTATE_READY ||
	       mcursor->processingState==MSTATE_RESERVED) {
      /* success: a due block was found, return it */
      *nexttime=acttime;
      break;
    }
  }
  if (mcursor) {
    /* prepare for scheduling and relocation */
    mcursor->processingState = MSTATE_IN_PROGRESS;
    mcursor->nextSampleDue = acttime + mcursor->sampleInterval - 
      (mh->mlsynclevel == 1 ? acttime%mcursor->sampleInterval : 0); 
  }
  pthread_mutex_unlock(&(mh->mutex));  
  return mcursor;
}

/*
 * ML_Relocate is used to move the metric block into a new list 
 * position according to it's next sample due time.
 * As metric blocks are always moved "to the right" next due date
 * is always in the future, we are searching only the list part
 * following the original position.
 * In case of insertions we assume the list head as initial position
 */

int ML_Relocate(ML_Head mlhead, MetricBlock *mblock)
{
  MetricBlock *mcursor, *mpredecessor;
  MutexML_Head *mh = (MutexML_Head*) mlhead; 

  if (!mblock)
    return 1;
  if (pthread_mutex_lock(&(mh->mutex)))
    return -1; /* invalid mutex */

  if (mblock->processingState==MSTATE_DELETED) { 
    /* deallocate storage for deleted block */
    free(mblock);    
  } else {
    if (mblock->processingState == MSTATE_CREATED) {
      /* special case 1: first insertion after creation */
      if (mh->mlsynclevel == 1) {
	/* synchronize with wallclock */
	time_t t = time(NULL);
	mblock->nextSampleDue = t - t%mblock->sampleInterval + mblock->sampleInterval;
      } else {
	/* schedule asap */
	mblock->nextSampleDue = 0;
      }
      /* equivalent to list head position but no need to unchain */
      mcursor = mh->first;
      mpredecessor = NULL;
    } else {
      /* locate the block in the list and unchain it */
      mcursor=mh->first;
      if (mcursor == mblock) {
	/* special case 2: found at list head */
	mh->first = mblock->nextMetric;
	mcursor = mh->first;
	mpredecessor = NULL;
      } else {  
	while (mcursor && mcursor->nextMetric != mblock) {
	  mcursor=mcursor->nextMetric;
	}
	if (!mcursor) {
	  return -1; /* error - could not locate metric block in list */
	} else {
	  mpredecessor = mcursor;
	  mpredecessor->nextMetric = mblock->nextMetric;
	  mcursor=mcursor->nextMetric;
	}
      }
    }
    /* now find new location for block */
    while (1) {
      if (mcursor==NULL || mblock-> nextSampleDue < mcursor->nextSampleDue) {
	if (mpredecessor == NULL) {
	  /* at list head */
	  mblock->nextMetric = mh->first;
	  mh->first = mblock;
	} else {
	  mblock->nextMetric = mcursor;
	  mpredecessor->nextMetric = mblock;
	}
	mblock->processingState = MSTATE_READY;
	break;
      }
      mpredecessor=mcursor;
      mcursor=mcursor->nextMetric;
    }
  }
  pthread_mutex_unlock(&(mh->mutex));  
  return 0;
}

int ML_Remove(ML_Head mlhead, int id)
{
  MetricBlock *mcursor, *mpredecessor;
  MutexML_Head *mh = (MutexML_Head*) mlhead; 
  if (pthread_mutex_lock(&(mh->mutex)))
    return -1; /* invalid mutex */ 
  mcursor=mh->first;
  if (mcursor && mcursor->metricId == id) {
    mh->first=mcursor->nextMetric;
    if (mcursor->processingState==MSTATE_IN_PROGRESS)
      /* is in progress - only mark for deletion*/
      mcursor->processingState=MSTATE_DELETED;
    else
      free(mcursor);
  } else 
    while(mcursor) {
      mpredecessor=mcursor;
      mcursor=mcursor->nextMetric;
      if (mcursor && mcursor->metricId == id) {
	mpredecessor->nextMetric = mcursor->nextMetric;
	if (mcursor->processingState==MSTATE_IN_PROGRESS)
	  /* is in progress - only mark for deletion*/
	  mcursor->processingState=MSTATE_DELETED;
	else
	  free(mcursor);
	break;
      }
    }
  pthread_mutex_unlock(&(mh->mutex));  
  return 0;
}

MetricBlock* MakeMB(int metricId, void(*sample)(int),time_t interval)
{
  MetricBlock *mb = NULL;
  if (metricId && sample) {
    mb = malloc(sizeof(MetricBlock));
    mb->metricId = metricId;
    mb->sampleInterval = interval;
    mb->sampler = sample;
    mb->processingState = MSTATE_CREATED;
  }
  return mb;
}
