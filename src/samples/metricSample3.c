/*
 * $Id: metricSample3.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
#include <stdlib.h>

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
  metricDef.mdName="sample3";
  metricDef.mdReposPluginName="samples/librepositorySample3.so";
  metricDef.mdId=mr(pluginname,metricDef.mdName);
  metricDef.mdSampleInterval=20;
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
  static int  intbuffer;
  int         numvalue = 0;
  MetricValue *mv;
  if (mret==NULL)
    fprintf(stderr,"Returner pointer is NULL\n");
  else {
#ifdef DEBUG
    fprintf(stderr,"Sampling for metric id %d\n",mid);
#endif
    mv = malloc(sizeof(MetricValue) + sizeof(int));
    if (mv) {
      mv->mvId = metricDef.mdId;
      mv->mvResource = "sample3res";
      mv->mvTimeStamp = 0;
      mv->mvDataLength = sizeof(int);
      intbuffer = 42;
      mv->mvData = (char *)&intbuffer;
      numvalue = 1;
      mret(mv);
    }
  }
  return numvalue;
}

void metricDeallocator(void *v)
{
  free(v);
}
