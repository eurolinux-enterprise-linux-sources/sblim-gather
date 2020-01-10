/*
 * $Id: mreposl.c,v 1.15 2009/12/02 22:15:27 tyreld Exp $
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
 * Description: Local Metric Value Repository
 * This is an implementation of a history-less (well it caches 15 minutes)
 * and local repository. Others will follow.
 */

#include "mrepos.h"
#include "mrwlock.h"
#include "mreg.h" 
#include <mtrace.h> 
#include <mlog.h> 
#include <reposcfg.h> 
#include <commheap.h> 

#include <stdlib.h>
#include <limits.h>
#include <string.h>


/* TODO: must revise repository IF to not require resource listing */

int LocalMetricResources(int id, MetricResourceId ** resources, 
			 const char * resource, const char * system);
int LocalMetricFreeResources(int id, MetricResourceId * resources);
  

/* Local repository functions */

int LocalMetricAdd (MetricValue *mv);
int LocalMetricRetrieve (int mid, MetricResourceId *resource, int *resnum,
			 MetricValue ***mv, int **num, 
			 time_t from, time_t to, int maxnum);
int LocalMetricRelease (MetricValue **mv, int *num);
int LocalMetricRegisterCallback (MetricCallback *mcb, int mid, int state);

int LocalMetricShutdown();

static MetricRepositoryIF mrep = {
  "LocalRepository",
  LocalMetricAdd,
  LocalMetricRetrieve,
  LocalMetricRelease,
  LocalMetricRegisterCallback,
  LocalMetricResources,
  LocalMetricFreeResources,
  LocalMetricShutdown
};

MetricRepositoryIF *MetricRepository = &mrep;

MRWLOCK_DEFINE(LocalRepLock);

/*
 * the local repository consists of linked lists of values 
 * which are assumed to be ordered by timestamp
 */

typedef struct _MReposCallback {
  MetricCallback         *mrc_callback;
  struct _MReposCallback *mrc_next;
  int                     mrc_usecount;
} MReposCallback;

typedef struct _MReposValue {
  struct _MReposValue *mrv_next;
  MetricValue         *mrv_value;
} MReposValue;

#define LRID_COPY 1

typedef struct _LocalResourceId {
  char        *lrid_resource;
  char        *lrid_system;
  MReposValue *lrid_first;
  int          lrid_flags;
} LocalResourceId;

struct _MReposHdr {
  int               mrh_id;
  MReposCallback   *mrh_cb;
  LocalResourceId  *mrh_reslist;
  size_t            mrh_resnum;
} * LocalReposHeader = NULL;
size_t LocalReposNumIds = 0;


static int locateres(int idx, const char *, const char *, int read);

static int locateIndex(int mid);
static int addIndex(int mid);

static int pruneRepository();

static int _LocalMetricResources(int midx, LocalResourceId ** resources,
				 const char * resource, const char * system);
static int _LocalMetricFreeResources(int midx, LocalResourceId * resources);
static int _MetricRetrieveNoLock (COMMHEAP ch, int mid, LocalResourceId *resource, int resnum,
				  MetricValue **mv, int *num, 
				  time_t from, time_t to, int maxnum);

int LocalMetricAdd (MetricValue *mv)
{
  int              idx;  
  int              ridx;
  int              rc=-1;
  MReposValue     *mrv;
  
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
	  ("LocalMetricAdd %p (%d)", mv, mv?mv->mvId:-1));  
  /* obligatory prune check */
  pruneRepository();
  
  if (mv && MWriteLock(&LocalRepLock)==0) {  
    idx=locateIndex(mv->mvId);
    if (idx==-1) {
      idx=addIndex(mv->mvId);
    }
    if (idx!=-1) {
      ridx=locateres(idx,mv->mvResource, mv->mvSystemId, 0);
      mrv=malloc(sizeof(MReposValue));
      mrv->mrv_next=LocalReposHeader[idx].mrh_reslist[ridx].lrid_first;
      mrv->mrv_value=malloc(sizeof(MetricValue));
      memcpy(mrv->mrv_value,mv,sizeof(MetricValue));
      mrv->mrv_value->mvResource=strdup(mv->mvResource);
      mrv->mrv_value->mvData=malloc(mv->mvDataLength);
      memcpy(mrv->mrv_value->mvData,mv->mvData,mv->mvDataLength);
      mrv->mrv_value->mvSystemId=strdup(mv->mvSystemId);
      LocalReposHeader[idx].mrh_reslist[ridx].lrid_first=mrv;
      if (LocalReposHeader[idx].mrh_cb) {
	COMMHEAP        ch = ch_init();
	MetricValue   **mvList = ch_alloc(ch,sizeof(MetricValue*)*2);
	int            *num = ch_alloc(ch,sizeof(int)); 
	MReposCallback *cblist = LocalReposHeader[idx].mrh_cb;
	M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
		("LocalMetricAdd processing callbacks for mid=%d", mv->mvId));
	/* It is not really expensive to call the retrieval function 
	   as the new value is at the list head.
	   Further, we don't bother whether it's a point or interval
	   metric: we always return the previous element, if available.
	 */
	
	*mvList++ = (void*)ch; /* set COMMHEAP for later automatic release */
	_MetricRetrieveNoLock(ch,mv->mvId,LocalReposHeader[idx].mrh_reslist+ridx,1,
			      mvList,num,0,mv->mvTimeStamp,2);
	while (num > 0 && cblist && cblist->mrc_callback) {
	  cblist->mrc_callback(*mvList,*num);
	  cblist = cblist->mrc_next;
	}
	LocalMetricRelease(mvList,num);
      } 
      rc=0;
    }
    MWriteUnlock(&LocalRepLock);
  }
  return rc;
}

/*
 * Retrieves metric values for metric id mid,
 * a resource (if specified) in a given time range.
 * The MetricValue array must be freed by a call 
 * to LocalMetricRelease.
 *
 * Behavior:
 * I) maxnum <= 0
 *    i) to <= 0: retrieve everything starting with "from"
 *    ii) to > 0: retrieve everything between "from" and "to"
 * II) maxnum >0
 *    i) from >= to (!=0) retrieve "maxnum" entries starting with "from"  
 *    ii) from == 0 retrieve the newest "maxnum" entries 
 */

static int _MetricRetrieveNoLock (COMMHEAP ch, int mid, LocalResourceId *resource, int resnum,
				  MetricValue **mv, int *num, 
				  time_t from, time_t to, int maxnum)
{
  int          j,k;
  int          totalnum=0;
  MReposValue *mrv = NULL;  
  MReposValue *first;
  size_t       reslen;
  size_t       syslen;

  if (to==0 || to ==-1) {
    to=LONG_MAX;
  }
  if (maxnum<=0) {
    maxnum=INT_MAX;
  } else if (to==from) {
    /* maximum number requested: only "from" timestamp relevant */
    to=LONG_MAX;
  }
  for (k=0;k<resnum;k++) {
    num[k]=0;
    mrv=resource[k].lrid_first;
    reslen = strlen(resource[k].lrid_resource) + 1;
    syslen = strlen(resource[k].lrid_system) + 1;
    first=NULL;
    mv[k]=NULL;
    while(mrv && from <= mrv->mrv_value->mvTimeStamp) {
      if (to >= mrv->mrv_value->mvTimeStamp) {
	/* mark */
	if (num[k] < maxnum) {
	  if (first==NULL)
	    first=mrv;
	  num[k]+=1;
	  if (from ==0 && num[k]==maxnum)
	    break;
	} else {
	  if (first==NULL)
	    first=mrv;
	  else
	    first=first->mrv_next;
	}
      }
      mrv=mrv->mrv_next;
    }
    if (num[k]) {
      totalnum += num[k];
      mv[k]=ch_alloc(ch,num[k]*sizeof(MetricValue));	
      for (j=0, mrv=first; j<num[k]; mrv=mrv->mrv_next) {
	/* copy over */
	memcpy(&mv[k][j],mrv->mrv_value,sizeof(MetricValue));
	mv[k][j].mvData=ch_alloc(ch,mrv->mrv_value->mvDataLength);
	memcpy(mv[k][j].mvData,
	       mrv->mrv_value->mvData,
	       mrv->mrv_value->mvDataLength);
	mv[k][j].mvResource=ch_alloc(ch,reslen);
	memcpy(mv[k][j].mvResource,resource[k].lrid_resource,reslen);
	mv[k][j].mvSystemId=ch_alloc(ch,syslen);
	memcpy(mv[k][j].mvSystemId,resource[k].lrid_system,syslen);
	j+=1;
      }
    }
  }
  return totalnum;
}

int LocalMetricRetrieve (int mid, MetricResourceId *resource, int *rnum,
			 MetricValue ***mv, int **num, 
			 time_t from, time_t to, int maxnum)
{
  int              rc=-1;
  int              idx;
  LocalResourceId *rList;
  COMMHEAP         ch;
  
  if (resource && rnum && mv && num && from >=0 && MReadLock(&LocalRepLock)==0) {
    idx=locateIndex(mid);
    if (idx >= 0) {
      ch=ch_init();
      *rnum = _LocalMetricResources(idx,&rList,resource->mrid_resource,resource->mrid_system);
      *num=ch_alloc(ch,(*rnum)*sizeof(int));
      *mv=ch_alloc(ch,(*rnum+1)*sizeof(MetricValue*));
      *(*mv)++=(void*)ch; /* store COMMHEAP in first element */
      rc = _MetricRetrieveNoLock(ch,mid,rList,*rnum,
				 *mv,*num,from,to,maxnum);
      _LocalMetricFreeResources(idx,rList);
    }
    MReadUnlock(&LocalRepLock);
  }
  return rc;
}

int LocalMetricRelease (MetricValue **mv, int *num)
{
  COMMHEAP ch;
  if (mv && num) {
    ch = (COMMHEAP)mv[-1];
    ch_release(ch);
    return 0;
  }
  return -1;
}

int LocalMetricRegisterCallback (MetricCallback *mcb, int mid, int state)
{
  int idx;
  MReposCallback * cblist;
  MReposCallback * cbprev;
  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,("LocalMetricRegisterCallback=%p, %d, %d",mcb,mid,state));
  if (mcb == NULL || (state != 0 && state != 1) ) {
    M_TRACE(MTRACE_ERROR,MTRACE_REPOS,
	    ("LocalMetricRegisterCallback invalid parameter=%p, %d, %d",mcb,mid,state));
    return -1;
  } 
  idx = locateIndex(mid);
  if (idx == -1) {
    idx = addIndex(mid);
  }
  cblist = LocalReposHeader[idx].mrh_cb;
  cbprev = LocalReposHeader[idx].mrh_cb;
  if (state == MCB_STATE_REGISTER ) {
    while(cblist && cblist->mrc_callback) {
      if (mcb == cblist->mrc_callback) {
	break;
      }
      cblist = cblist->mrc_next;
    }
    if (cblist == NULL) {
      cblist = malloc(sizeof(MReposCallback));
      cblist->mrc_callback = mcb;
      cblist->mrc_next = LocalReposHeader[idx].mrh_cb;
      LocalReposHeader[idx].mrh_cb = cblist;
    }
    cblist->mrc_usecount ++;
  } else if ( state == MCB_STATE_UNREGISTER) {
    while(cblist && cblist->mrc_callback) {
      if (mcb == cblist->mrc_callback) {
	cblist->mrc_usecount--;
	break;
      }
      cbprev = cblist;
      cblist = cblist->mrc_next;
    }
    if (cblist && cblist->mrc_usecount == 0) {
      if (cbprev == LocalReposHeader[idx].mrh_cb) {
	LocalReposHeader[idx].mrh_cb = cblist->mrc_next;
      } else {
	cbprev->mrc_next=cblist->mrc_next;
      }
      free (cblist);
    }
  }
  return 0;
}

static int locateIndex(int mid)
{
  int idx=0;
  while (idx < LocalReposNumIds) {
    if (LocalReposHeader[idx].mrh_id == mid)
      return idx;
    else
      idx+=1;
  }
  return -1;
}

static int addIndex(int mid)
{
  LocalReposNumIds += 1;
  LocalReposHeader=realloc(LocalReposHeader,
			   LocalReposNumIds*sizeof(struct _MReposHdr));
  LocalReposHeader[LocalReposNumIds-1].mrh_reslist=NULL;
  LocalReposHeader[LocalReposNumIds-1].mrh_resnum=0;
  LocalReposHeader[LocalReposNumIds-1].mrh_cb=NULL;			   
  LocalReposHeader[LocalReposNumIds-1].mrh_id=mid;			   
  return LocalReposNumIds-1;
}


#define MIN_EXPIRATION_INTERVAL 60
#define MAX_EXPIRATION_INTERVAL 3600*24
#define DEFAULT_EXPIRATION_INTERVAL 1200

static int PRUNE_INTERVAL=0; /* 600 */
static int EXPIRATION_INTERVAL=0; /* 2 * PRUNE_INTERVAL */

int LocalMetricShutdown()
{
  int              i,j;
  MReposValue      *v, *bv;
  
  if (MWriteLock(&LocalRepLock)==0) {
    for (i=0; i<LocalReposNumIds; i++) {
      for (j=0; j<LocalReposHeader[i].mrh_resnum;j++) {
	bv = NULL;
	v = LocalReposHeader[i].mrh_reslist[j].lrid_first;
	/* now delete all metrics*/
	while (v) {
	  free(v->mrv_value->mvResource); 
	  free(v->mrv_value->mvData);
	  free(v->mrv_value->mvSystemId);
	  free(v->mrv_value);
	  bv=v;
	  v=v->mrv_next;
	  free(bv);
	}
	/* delete resource list */
	free(LocalReposHeader[i].mrh_reslist[j].lrid_resource);
	free(LocalReposHeader[i].mrh_reslist[j].lrid_system);
      }
      free(LocalReposHeader[i].mrh_reslist);
    }
    LocalReposNumIds = 0;
    free(LocalReposHeader);
    LocalReposHeader=NULL;
    MWriteUnlock(&LocalRepLock);
  }
  return 0;
}

static int pruneRepository()
{
  static time_t    nextPrune=0;
  size_t           numPruned=0;
  int              i,j;
  MReposValue      *v, *bv;
  char             cfgbuf[200];
  time_t           now=time(NULL);

  if (EXPIRATION_INTERVAL == 0) {
    if (reposcfg_getitem("ExpirationInterval",cfgbuf,sizeof(cfgbuf))==0) {
      EXPIRATION_INTERVAL=atoi(cfgbuf);
    }
    if (EXPIRATION_INTERVAL == 0) {
      EXPIRATION_INTERVAL = DEFAULT_EXPIRATION_INTERVAL;
    }
    if (EXPIRATION_INTERVAL < MIN_EXPIRATION_INTERVAL) {
      m_log(M_ERROR,M_SHOW,"Expiration interval %d too small, adjusting to %d.\n",
	    EXPIRATION_INTERVAL, MIN_EXPIRATION_INTERVAL);
      EXPIRATION_INTERVAL = MIN_EXPIRATION_INTERVAL;
    }
    if (EXPIRATION_INTERVAL > MAX_EXPIRATION_INTERVAL) {
      m_log(M_ERROR,M_SHOW,"Expiration interval %d too large, adjusting to %d.\n",
	    EXPIRATION_INTERVAL, MAX_EXPIRATION_INTERVAL);
      EXPIRATION_INTERVAL = MAX_EXPIRATION_INTERVAL;
    }
    PRUNE_INTERVAL=EXPIRATION_INTERVAL/2;
    M_TRACE(MTRACE_DETAILED,MTRACE_REPOS,
	    ("Setting local repository intervals, expire: %d, prune:%d.",
	     EXPIRATION_INTERVAL, PRUNE_INTERVAL));
  }

  if (nextPrune<now) {
    nextPrune = now + PRUNE_INTERVAL;
    if (MWriteLock(&LocalRepLock)==0) {
      for (i=0; i<LocalReposNumIds; i++) {
	for (j=0; j<LocalReposHeader[i].mrh_resnum;j++) {
	  bv=NULL;
	  v = LocalReposHeader[i].mrh_reslist[j].lrid_first;
	  while (v && v->mrv_value->mvTimeStamp > now - EXPIRATION_INTERVAL) {
	    bv = v;
	    v = v->mrv_next;
	  }
	  if (v && v->mrv_value->mvTimeStamp <= now - EXPIRATION_INTERVAL) {
	    if (bv==NULL)
	      LocalReposHeader[i].mrh_reslist[j].lrid_first = NULL;
	    else
	      bv->mrv_next = NULL;
	    /* now delete all outdated metrics*/
	    while (v) {
	      free(v->mrv_value->mvResource); 
	      free(v->mrv_value->mvData);
	      free(v->mrv_value->mvSystemId);
	      free(v->mrv_value);
	      bv=v;
	      v=v->mrv_next;
	      free(bv);
	      numPruned++;
	    }
	  }
	}
      }
      MWriteUnlock(&LocalRepLock);
    }
  }
  return numPruned;
}


static inline int rescmp(LocalResourceId *rid, const char *r, size_t rl, const char *s, size_t rs)
{
  int state = memcmp(rid->lrid_resource,r,rl);
  if (state) return state;
  return memcmp(rid->lrid_system,s,rs);
}

static int locateres(int idx, const char *res, const char *sys, int read)
{
  /* this one is called quite often - therefore optimized */
  int left, right, cursor;
  int dir;
  size_t reslen = strlen(res)+1;
  size_t syslen = strlen(sys)+1;
  
  left=0;
  right=LocalReposHeader[idx].mrh_resnum-1;
  while(left <= right) {
    cursor=(right+left)/2;
    dir = rescmp(LocalReposHeader[idx].mrh_reslist + cursor,res,reslen,sys,syslen);
    if (dir<0) {
      left = cursor+1;
    } else if (dir>0) {
      right = cursor-1;
    } else {
      /* found: search no more */
      return cursor;
    }
  }
  
  if (!read) {
	  /* insert new resource */
	  LocalReposHeader[idx].mrh_reslist = 
	    realloc(LocalReposHeader[idx].mrh_reslist,
		    (LocalReposHeader[idx].mrh_resnum+1)*sizeof(LocalResourceId));

	  memmove(LocalReposHeader[idx].mrh_reslist+left+1,
		  LocalReposHeader[idx].mrh_reslist+left, 
		  sizeof(LocalResourceId)*(LocalReposHeader[idx].mrh_resnum-left));

	  LocalReposHeader[idx].mrh_reslist[left].lrid_resource=malloc(reslen);
	  memcpy(LocalReposHeader[idx].mrh_reslist[left].lrid_resource,res,reslen);
	  LocalReposHeader[idx].mrh_reslist[left].lrid_system=malloc(syslen);
	  memcpy(LocalReposHeader[idx].mrh_reslist[left].lrid_system,sys,syslen);
	  LocalReposHeader[idx].mrh_reslist[left].lrid_first=NULL;
	  LocalReposHeader[idx].mrh_reslist[left].lrid_flags=0;
	  LocalReposHeader[idx].mrh_resnum ++;
	  M_TRACE(MTRACE_FLOW,MTRACE_REPOS,
		  ("LocalMetricAdd adding resource %s:%s (%d)", 
		   sys,res, LocalReposHeader[idx].mrh_id));  
	  return left;
	}
	
	return -1;
}

static int _LocalMetricResources(int midx, LocalResourceId ** resources,
				 const char * resource, const char * system)
{
  /* internal version of resource lister 
   * only fully qualified or fully unqualified searches allowed (temporary)
   */
  int ridx;
  int i;
  int num;

  *resources = NULL;
  if (resource==NULL && system==NULL) {
    *resources = LocalReposHeader[midx].mrh_reslist;
    return LocalReposHeader[midx].mrh_resnum;
  } else if (resource && system) {
    ridx = locateres(midx,resource,system, 1);
    if (ridx < 0) {
    	return 0;
    }
    *resources = LocalReposHeader[midx].mrh_reslist + ridx;
    return 1;
  } else {
    /* wasting some memory for speed */
    *resources = calloc(LocalReposHeader[midx].mrh_resnum,sizeof(LocalResourceId));
    num = 0;
    for (i=0; i < LocalReposHeader[midx].mrh_resnum; i++) {
      if (i==0) {
	(*resources)[0].lrid_flags = LRID_COPY;
      }
      if (resource && 
	  strcmp(LocalReposHeader[midx].mrh_reslist[i].lrid_resource,resource)) {
	continue;
      }
      if (system && 
	  strcmp(LocalReposHeader[midx].mrh_reslist[i].lrid_system,system)) {
	continue;
      }
      num ++;
      (*resources)[i] = LocalReposHeader[midx].mrh_reslist[i];
    } 
    return num;
  }
  return 0;
}

static int _LocalMetricFreeResources(int midx, LocalResourceId * resources)
{
  if (resources && resources[0].lrid_flags & LRID_COPY) {
    free(resources);
  }
  return 0;
}

int LocalMetricResources(int mid, MetricResourceId ** resources,
			 const char * resource, const char * system)
{
  int i;
  int num;
  int idx;
  LocalResourceId *lresources;
  /*
   * dangerous - must always create a copy !!!
   * must be locked as well!
   */
  lresources = NULL;
  if (MReadLock(&LocalRepLock)) {
    return 0;
  }
  idx = locateIndex(mid);
  if (idx == -1) {
    MReadUnlock(&LocalRepLock);
    return 0;
  }
  num=_LocalMetricResources(idx,&lresources,resource,system);
  *resources=calloc(num+1,sizeof(MetricResourceId));  
  for (i=0; i < num; i++) {
    (*resources)[i].mrid_resource = lresources[i].lrid_resource;
    (*resources)[i].mrid_system = lresources[i].lrid_system;
  }
  (*resources)[num].mrid_resource=NULL;
  _LocalMetricFreeResources(idx,lresources);
  MReadUnlock(&LocalRepLock);
  return num;
}

int LocalMetricFreeResources(int mid, MetricResourceId * resources)
{
  int idx;
  
  if (MReadLock(&LocalRepLock)==0) {
    idx = locateIndex(mid);
    if (resources && idx != -1) {
      free (resources);
    }
    MReadUnlock(&LocalRepLock);
  }
  return 0;
}
