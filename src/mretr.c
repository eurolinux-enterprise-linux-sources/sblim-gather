/*
 * $Id: mretr.c,v 1.3 2009/05/20 19:39:55 tyreld Exp $
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
 * Description: Threaded metric retriever.
 * This module offers services to process a metric block list with
 * multiple threads in parallel.
 * Two kinds of threads are active: one controller and multiple retrievers.
 * The retrievers use the metric block list functions to schedule metrics
 * processing.
 *
 */

#include "mretr.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define CMD_REGULAR 0
#define CMD_STOPONE 1
#define CMD_STOPALL 2
#define CMD_TERMINATE 2
#define CMD_WAKEUP 3

typedef struct _MR_Control {
  ML_Head  mlhead;
  unsigned maxRetrieverNum;
  unsigned retrieverNum;
  int      retrieverCmd;
  pthread_mutex_t mutex;
  pthread_cond_t condController;
  pthread_cond_t condRetriever;
  pthread_t controller;
  int      controllerCmd;
} MR_Control;

static void * MR_Controller(void * arg);
static void * MR_Retriever(void * arg);
static int startRetrievers(MR_Control *mc);
static int sleepuntil(MR_Control *mc, time_t nextwakeup);

MR_Handle MR_Init(ML_Head mhead, unsigned numthreads)
{
  /* initialize control structure */
  MR_Control *mc = malloc(sizeof(MR_Control));

  mc->mlhead=mhead;
  mc->retrieverNum=0;
  mc->maxRetrieverNum=numthreads;
  mc->retrieverCmd=CMD_REGULAR;
  mc->controllerCmd=CMD_REGULAR;
  pthread_mutex_init(&(mc->mutex),NULL);
  pthread_cond_init(&(mc->condController),NULL);
  pthread_cond_init(&(mc->condRetriever),NULL);
  if (pthread_create(&(mc->controller),NULL,MR_Controller,mc)==0)
    return mc;
  else {
    /* thread failure - must clean up */
    MR_Finish(mc);
    return NULL;
  }
}

void MR_Finish(MR_Handle mrh)
{
  MR_Control *mc=mrh;
  
  if (!mc) return;

#ifdef DEBUG
  fprintf(stderr,"Finish Requested.");
#endif
  if (pthread_mutex_lock(&(mc->mutex))==0) {
    /* signal to controller that we want termination */
    mc->controllerCmd=CMD_TERMINATE;
    pthread_cond_signal(&(mc->condController));
    pthread_mutex_unlock(&(mc->mutex));
  }
  /* wait for controller to exit - then deallocate everything */
  pthread_join(mc->controller,NULL); 
#ifdef DEBUG
  fprintf(stderr,"Finished after controller shutdown.\n");
#endif
  pthread_cond_destroy(&(mc->condController));
  pthread_cond_destroy(&(mc->condRetriever));
  pthread_mutex_destroy(&(mc->mutex));
  mc->mlhead=NULL;
  free(mc);
}


void MR_Wakeup(MR_Handle mrh)
{
  MR_Control *mc=mrh;
  
  if (!mc) return;

#ifdef DEBUG
  fprintf(stderr,"Wakeup Requested.");
#endif
  if (pthread_mutex_lock(&(mc->mutex))==0) {
    /* signal to controller that we want to wakeup the retrievers */
    mc->controllerCmd=CMD_WAKEUP;
    pthread_cond_signal(&(mc->condController));
    pthread_mutex_unlock(&(mc->mutex));
  }
  
}

static void * MR_Controller(void * arg)
{
  MR_Control *mc = arg;
  
  if(!mc) return NULL;

  if (startRetrievers(mc)==0 && pthread_mutex_lock(&(mc->mutex))==0) {
#ifdef DEBUG
    fprintf(stderr,"Cotroller entering wait\n");
#endif
    /* main loop -- normally we remain in the condwait state */
    while(mc->controllerCmd!=CMD_TERMINATE) {
      pthread_cond_wait(&(mc->condController),&(mc->mutex));
#ifdef DEBUG
      fprintf(stderr,"Controller woke up, cmd=%d, active retrievers=%d\n",
	      mc->controllerCmd,mc->retrieverNum);
#endif
      if (mc->controllerCmd==CMD_WAKEUP) {
	/* wake up the receivers */
	mc->controllerCmd=CMD_REGULAR;
	mc->retrieverCmd=CMD_WAKEUP;
	pthread_cond_broadcast(&(mc->condRetriever));	
      }
    }
    pthread_mutex_unlock(&(mc->mutex));
  }
  /* termination loop */
#ifdef DEBUG
  fprintf(stderr,"Controller shutdown requested\n");
#endif
  if (pthread_mutex_lock(&(mc->mutex))==0) {
    mc->retrieverCmd=CMD_STOPALL;
    pthread_cond_broadcast(&(mc->condRetriever));
    while (mc->retrieverNum) {
      pthread_cond_wait(&(mc->condController),&(mc->mutex));
#ifdef DEBUG
      fprintf(stderr,"Controller: number of active retrievers: %d\n",mc->retrieverNum);
#endif
    }
    pthread_mutex_unlock(&(mc->mutex));
  }
  return NULL;
}

static void * MR_Retriever(void * arg)
{
  MR_Control *mc = arg;
  MetricBlock *mb;
  time_t currentTime, nextTime;
  int retrieverid;

  if(!mc) return NULL;

#ifdef DEBUG
  fprintf(stderr,"Retriever thread created.\n");
#endif
  if (pthread_mutex_lock(&(mc->mutex))==0) {
    mc->retrieverNum+=1; /* make us visible */
    retrieverid=(int)pthread_self();
#ifdef DEBUG
    fprintf(stderr,"Retriever %d starting ...\n",retrieverid);
#endif
    while (1) {
      if (mc->retrieverCmd==CMD_WAKEUP)
	mc->retrieverCmd=CMD_REGULAR; /* already awake - continue */
      if (mc->retrieverCmd!=CMD_REGULAR)
	break; /* shutdown requested */
      pthread_mutex_unlock(&(mc->mutex)); /* release control */
      currentTime=time(NULL);
      mb=ML_SelectNextDue(mc->mlhead,currentTime,&nextTime,1);
      if (nextTime<0) {
#ifdef DEBUG
	fprintf(stderr,"Retriever %d failed to get next sample time, going to sleep \n",retrieverid);
#endif
	if (sleepuntil(mc,0)<0) {
	  fprintf(stderr,"Retriever %d failure in sleepuntil!\n",retrieverid);
	  break; /* oops */
	}
      } else if (mb) {
#ifdef DEBUG
	fprintf(stderr,"Retriever %d sample metric id %d\n",retrieverid,mb->metricId);
#endif
	mb->sampler(mb->metricId);
	if (ML_Relocate(mc->mlhead,mb)) 
	  fprintf(stderr,"Retriever %d failed relocating metric block.",retrieverid);
      } else {
#ifdef DEBUG
	fprintf(stderr,"Retriever %d taking a nap for approx. %ld seconds\n",
		retrieverid,nextTime-currentTime);
#endif
	if (sleepuntil(mc,nextTime)<0) {
	  fprintf(stderr,"Retriever %d failure in sleepuntil!\n",retrieverid);
	  break; /* oops */
	}
      }
      if (pthread_mutex_lock(&(mc->mutex)))
	break; /* invalid mutex, leave */
    }
    /* cleanup */
#ifdef DEBUG
    fprintf(stderr,"Retriever %d shutdown...\n",retrieverid);
#endif
    if (mc->retrieverCmd==CMD_STOPONE)
      mc->retrieverCmd=CMD_REGULAR; /* this thread is taking the sacrifice */
    mc->retrieverNum-=1; /* we are gone */
    pthread_mutex_unlock(&(mc->mutex));
    pthread_cond_signal(&(mc->condController)); /* wake up controller */
    return NULL;
  } else {
    fprintf(stderr,"couldn't aquire mutex - going down\n.");
    return NULL; /* no need for signals: this one went entirely wrong */
  }
}

static int startRetrievers(MR_Control *mc)
{
  pthread_t  thread;
  pthread_attr_t  threadattr;
  int i;
 
  /* start up the maximum nuber of retrievers */
  if (pthread_mutex_lock(&(mc->mutex))==0) {
    /* we remain optimistic and do not check for errors */
    pthread_attr_init(&threadattr);
    pthread_attr_setdetachstate(&threadattr,PTHREAD_CREATE_DETACHED);
    for (i=mc->retrieverNum; i < mc->maxRetrieverNum;i++)
      pthread_create(&thread,&threadattr,MR_Retriever,mc);
    pthread_attr_destroy(&threadattr);    
    pthread_mutex_unlock(&(mc->mutex));
    return 0;
  } else {
    fprintf(stderr,"error acquiring mutex\n");
    return -1;
  }
}

static int sleepuntil(MR_Control *mc, time_t nextwakeup)
{
  time_t acttime;
  struct timespec ts;
  
  /* perform a timed wait for the next metric block */
  ts.tv_sec=nextwakeup;
  ts.tv_nsec=0;
  if (pthread_mutex_lock(&(mc->mutex))==0) {  
    if (nextwakeup==0) 
      /* wait untimed */
      pthread_cond_wait(&(mc->condRetriever),
			&(mc->mutex));
    else 
      while ((acttime=time(NULL)) < nextwakeup &&
	     mc->retrieverCmd==CMD_REGULAR)
	pthread_cond_timedwait(&(mc->condRetriever),
			       &(mc->mutex),&ts);
    if (mc->retrieverCmd==CMD_WAKEUP)
      mc->retrieverCmd=CMD_REGULAR;
    pthread_mutex_unlock(&(mc->mutex));
    return 0;
  } else
    return -1;
}
