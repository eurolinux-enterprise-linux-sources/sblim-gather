/*
 * $Id: mlist.h,v 1.3 2009/05/20 19:39:55 tyreld Exp $
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
 * Description:  List of Metric Blocks.
 * Functions for Metric Block List Processing.
 */

#ifndef MLIST_H
#define MLIST_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MetricBlock MetricBlock;
struct _MetricBlock {
  int    metricId;
  time_t sampleInterval;
  time_t nextSampleDue;
  int    processingState;
  void  (*sampler)(int id);
  MetricBlock *nextMetric;
}; 

typedef void* ML_Head;

#define MSTATE_READY       0
#define MSTATE_IN_PROGRESS 1
#define MSTATE_CREATED     2
#define MSTATE_RESERVED    3
#define MSTATE_DELETED     4

ML_Head ML_Init(int synclevel);
int ML_Finish(ML_Head mlhead);
int ML_Reset(ML_Head mlhead);
MetricBlock* ML_SelectNextDue(ML_Head mlhead, time_t acttime, 
			      time_t *nexttime, int reserve);
int ML_Relocate(ML_Head mlhead, MetricBlock *mblock);
int ML_Remove(ML_Head mlhead, int id);
MetricBlock* MakeMB(int metricId, void (*sample)(int), time_t sampleInterval);

#ifdef __cplusplus
}
#endif

#endif
