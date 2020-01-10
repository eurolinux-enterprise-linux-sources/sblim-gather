/*
 * $Id: metricMassive.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Sample Code
 */

#include <mplugin.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

static MetricDefinition metricDef;

static MetricRetriever   metricRetriever;
static MetricDeallocator metricDeallocator;

int _DefinedMetrics (MetricRegisterId *mr,
		     const char *pluginname,
		     size_t *mdnum,
		     MetricDefinition **md)
{
#ifdef DEBUG  
  fprintf(stderr,"Retrieving metric definitions\n");
#endif
  if (mr==NULL||mdnum==NULL||md==NULL) {
    fprintf(stderr,"invalid parameter list\n");
    return -1;
  }
  metricDef.mdVersion=MD_VERSION;
  metricDef.mdName="massive";
  metricDef.mdReposPluginName="librepositoryMassive.so";
  metricDef.mdId=mr(pluginname,metricDef.mdName);
  metricDef.mdSampleInterval=300;
  metricDef.mproc=metricRetriever;
  metricDef.mdeal=metricDeallocator;
  *mdnum=1;
  *md=&metricDef;
  return 0;
}

int _StartStopMetrics (int starting) 
{
#ifdef DEBUG
  fprintf(stderr,"%s metric processing\n",starting?"Starting":"Stopping");
#endif
  return 0;
}

int metricRetriever(int mid, MetricReturner mret)
{
  int         numvalue = 100000, i;  
  MetricValue *mv;

  if (mret==NULL)
    fprintf(stderr,"Returner pointer is NULL\n");
  else {
#ifdef DEBUG
    fprintf(stderr,"Sampling for metric id %d\n",mid);
#endif
    for (i=0; i < numvalue; i++) {
	mv = calloc(sizeof(MetricValue),1);
	mv->mvId = metricDef.mdId;
	mv->mvResource = malloc(strlen("massive-res")+strlen(" 1234567890") +1 );;
	sprintf(mv->mvResource,"%s-%d","massive-res",i);
	mv->mvTimeStamp = time(NULL);
	mv->mvData = malloc(sizeof(time_t));
	*(time_t*)mv->mvData = htonl(time(NULL));
	mv->mvDataLength = sizeof(time_t);
	mret(mv);
    }
  }
  return numvalue;
}

void metricDeallocator(void *v)
{
  MetricValue *mv = (MetricValue*)v;
#ifdef DEBUG
     fprintf(stderr,"Deallocating metric value for id %d\n",mv->mvId);
#endif
  free(mv->mvResource);
  free(mv->mvData);
  free(mv);
}
