/*
 * $Id: gather.c,v 1.12 2009/05/20 19:39:55 tyreld Exp $
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
 * Contributors: Michael Schuele <schuelem@de.ibm.com> 
 *
 * Description: Gatherer Library
 * 
 * Runtime Control Functions.
 * For now we don't have synchronization features as we assume only one
 * control thread.
 */

#include "gather.h"
#include "mreg.h"
#include "mplugmgr.h"
#include "mlist.h"
#include "mretr.h"
#include "mrepos.h"
#include <mlog.h>
#include <gathercfg.h>
#include <dirutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef GATHER_PLUGINDIR
#define GATHER_PLUGINDIR "/usr/lib/gather/mplug"
#endif

#define SYNCLEVEL_MIN     0
#define SYNCLEVEL_MAX     1
#define SYNCLEVEL_DEFAULT 1

#define min(a,b) ((a) < (b) ? (a) : (b))

/* Plugin Control */
typedef struct _PluginList {
  MetricPlugin       *plugin;
  struct _PluginList *next;
} PluginList;

static PluginList *pluginhead=NULL;
static size_t      pluginnum=0;

static void pl_link(MetricPlugin *);
static void pl_unlink(MetricPlugin *);
static MetricPlugin* pl_find(const char *);
static MetricPlugin* pl_first();
static MetricPlugin* pl_next(const MetricPlugin*);

/* Retriever Control */
static int globalInterval=-1;
static ML_Head   *metriclist=NULL;
static MR_Handle *retriever=NULL;

/* Sample Function */
static void gather_sample(int id);

/* Gatherer Interface Implementation */
int gather_init()
{
  /* Allocate all the data structures needed
     - plugin registry
     - retrievers 
  */
  char cfgbuf[500];
  char *pluginpath;
  char *cfgidx1;
  char *cfgidx2;
  int  synclevel = SYNCLEVEL_DEFAULT;

  if (metriclist) return -1;
  pluginhead = NULL;
  MPR_InitRegistry();
  if (gathercfg_getitem("Synchronization",cfgbuf,sizeof(cfgbuf))==0) {
    if (sscanf(cfgbuf,"%d",&synclevel) != 1 || 
	synclevel < SYNCLEVEL_MIN || synclevel > SYNCLEVEL_MAX) {
      synclevel = SYNCLEVEL_DEFAULT;
      m_log(M_ERROR,M_SHOW,
	    "Invalid synchronization value specified: %s. Ignoring it.\n", cfgbuf);
    }
  }
  metriclist = ML_Init(synclevel);
  if (gathercfg_getitem("SampleInterval",cfgbuf,sizeof(cfgbuf))==0) {
    if (sscanf(cfgbuf,"%d",&globalInterval) != 1) {
      m_log(M_ERROR,M_SHOW,
	    "Invalid sample interval specified: %s. Ignoring it.\n", cfgbuf);
    }
  }
  if (gathercfg_getitem("PluginDirectory",cfgbuf,sizeof(cfgbuf))==0) {
    pluginpath=cfgbuf;
  } else {
    pluginpath=GATHER_PLUGINDIR;
  }
  MP_SetPluginPath(pluginpath);
  if (gathercfg_getitem("AutoLoad",cfgbuf,sizeof(cfgbuf))==0) {
    if (strcmp(cfgbuf,"*")==0) {
      /* load all plugins */
      metricplugin_loadall();
    } else {
      /* load all specified plugins */
      for (cfgidx1=cfgbuf;cfgidx1;cfgidx1=cfgidx2) {
	cfgidx2 = strchr(cfgidx1,':');
	if (cfgidx2) {
	  *cfgidx2++=0;
	}
	metricplugin_add(cfgidx1);
      }      
    }
  }
  return 0;
}

int gather_terminate()
{
  /* stop everything */
  if (metriclist==NULL) return -1;
  gather_stop();
  while (pluginhead) {
    metricplugin_remove(pluginhead->plugin->mpName);
  };
  ML_Finish(metriclist);
  metriclist=NULL;
  MPR_FinishRegistry();
  return 0;
}

int gather_start()
{
  /* start the retrieval with one thread */
  if (metriclist && retriever==NULL) {
    ML_Reset(metriclist);
    retriever=MR_Init(metriclist,1);
    return 0;
  } else {
    return -1;
  }
}

int gather_stop()
{
  /* stop the retrieval */
  if (retriever) {
    MR_Finish(retriever);
    retriever=NULL;
    return 0;
  } else{
    return -1;
  }
}

void gather_status(GatherStatus *gs)
{
  if (gs) {
    PluginList *p=pluginhead;
    gs->gsNumPlugins=gs->gsNumMetrics=0;
    while(p && p->plugin) {
      gs->gsNumPlugins+=1;
      gs->gsNumMetrics+=p->plugin->mpNumMetricDefs;
      p=p->next;
    }
    gs->gsInitialized=metriclist!=NULL;
    gs->gsSampling=retriever!=NULL;
  }
}

int metricplugin_loadall()
{
  char ** plugins;
  int i, rc=0;
  if (getfilelist(MP_GetPluginPath(),"*.so",&plugins) == 0) {
    for (i=0; plugins[i] && rc==0; i++) {
      rc=metricplugin_add(plugins[i]);
    }
    releasefilelist(plugins);
  }
  return rc;
}

int metricplugin_add(const char *pluginname)
{
  MetricPlugin *mp;
  size_t i;
  int status = -1;
  if (metriclist && pluginname && pl_find(pluginname)==NULL) {
    mp = malloc(sizeof(MetricPlugin));
    /* load plugin */
    mp->mpName = strdup(pluginname);
    mp->mpRegister=MPR_IdForString;
    if (MP_Load(mp)==0) {
      status = 0;
      pl_link(mp);
      /* register all metrics */
      for (i=0;i<mp->mpNumMetricDefs;i++) {
	if (mp->mpMetricDefs[i].mdVersion > 
	    (MD_VERSION_MAJOR+MD_VERSION_MINOR_MAX) || 
	    mp->mpMetricDefs[i].mdReposPluginName==NULL) {
	  status=-1;
	  break;
	} 
	if (MPR_UpdateMetric(pluginname,mp->mpMetricDefs+i)==0 &&
	    mp->mpMetricDefs[i].mproc) {	  
	  MetricBlock *mb=MakeMB(mp->mpMetricDefs[i].mdId,
				 gather_sample,
				 ( (globalInterval > 0) ?
				   globalInterval :
				   mp->mpMetricDefs[i].mdSampleInterval));
	  if (mb==NULL || ML_Relocate(metriclist,mb)) {
	    status = -1;
	    break;
	  }
	  else if (retriever)
	    /* notify retriever about new metrics */
	    MR_Wakeup(retriever);
	}
      }
      if (status<0) {
      /* failed during registration - unload */
	metricplugin_remove(pluginname);
	return status;
      }
    } else {
      /* failed during loading */
      if(mp->mpName) free(mp->mpName);
      if(mp) free(mp);
    }
  }
  return status;
}

int metricplugin_remove(const char *pluginname)
{
  MetricPlugin *mp;
  size_t i;
  int status = -1;
  if (metriclist && pluginname) {
    mp = pl_find(pluginname);
    if (mp) {
      /* unregister all metrics for this plugin */
      for (i=0;i<mp->mpNumMetricDefs;i++) {
	if (mp->mpMetricDefs[i].mproc) {
	  ML_Remove(metriclist,mp->mpMetricDefs[i].mdId);
	}
	MPR_RemoveMetric(mp->mpMetricDefs[i].mdId);
      }
      pl_unlink(mp);
      MP_Unload(mp);
      free(mp->mpName);
      free(mp);
      status = 0;
    }
  }
  return status;
}

int metricplugin_list(const char *pluginname, PluginDefinition **pdef, 
		      COMMHEAP ch)
{
  MetricPlugin *mp;
  size_t i;
  if (metriclist && pluginname && pdef) {
    mp = pl_find(pluginname);
    if (mp) {
      *pdef = ch_alloc(ch,sizeof(PluginDefinition)*mp->mpNumMetricDefs);
      /* store all metric infos for this plugin */
      for (i=0;i<mp->mpNumMetricDefs;i++) {
	(*pdef)[i].pdId=mp->mpMetricDefs[i].mdId;
	(*pdef)[i].pdName=mp->mpMetricDefs[i].mdName;
      }
      return i;
    }
  }
  return -1;
}

int metricpluginname_list(char ***pluginname, 
			  COMMHEAP ch)
{
  MetricPlugin *mp = NULL;
  if (pluginname) {
    int i=0;
    *pluginname = ch_alloc(ch,sizeof(char*)*(pluginnum+1));
    mp = pl_first();
    while (mp) {
      (*pluginname)[i]= ch_alloc(ch,strlen(mp->mpName)+1);
      strcpy((*pluginname)[i],mp->mpName);
      i += 1;
      mp = pl_next(mp);
    }
    (*pluginname)[i]=NULL;
    return 0;
  }
  return -1;
}

static MetricPlugin * pl_first()
{
  return pluginhead?pluginhead->plugin:NULL;
}

static MetricPlugin * pl_next(const MetricPlugin *mp)
{
  /* not very efficient */
  PluginList *p = pluginhead;
  while(p) {
    if (p->plugin==mp)
      return p->next->plugin;
  }
  return NULL;
}

static void pl_link(MetricPlugin *mp)
{
  PluginList *p = pluginhead;
  if (p == NULL) {
    pluginhead = malloc(sizeof(PluginList));
    p = pluginhead;
  } else {
    while (p->next)
      p=p->next;
    p->next=malloc(sizeof(PluginList));
    p=p->next;
  }
  p->plugin = mp;
  p->next = NULL;
  pluginnum+=1;
}

static void pl_unlink(MetricPlugin *mp)
{
  PluginList *p, *q;
  p = pluginhead;
  if (p && p->plugin==mp) {
    pluginhead=p->next;
    free(p);
    pluginnum-=1;
  } else
    while (p->next) {
      if (p->next->plugin==mp) {
	q=p->next;
	p->next=q->next;
	free(q);
	pluginnum-=1;
	break;
      }
      p=p->next;
    }
}

static MetricPlugin* pl_find(const char *name)
{
  PluginList *p = pluginhead;
  while(p) {
    if (strcmp(p->plugin->mpName,name)==0)
      break;
    p=p->next;
  }
  return p?p->plugin:NULL;
}

static void gather_sample(int id)
{
  MetricDefinition *md;
  
  md=MPR_GetMetric(id);
  if (md && md->mproc) {
    md->mproc(id,MetricRepository->mrep_add);
  }
}
