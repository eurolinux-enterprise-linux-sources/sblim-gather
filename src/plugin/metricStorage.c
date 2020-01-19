/*
 * $Id: metricStorage.c,v 1.2 2011/11/17 01:06:35 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2011
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Tyrel Datwyler <tyreld@us.ibm.com>
 * Contributors: 
 *
 * Description:
 * Metrics Gatherer Plugin of the following Storage specific 
 * metrics :
 *
 * _BlockStorage
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <time.h>
#include <limits.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef ULLONG_MAX
#   define ULLONG_MAX 18446744073709551615ULL
#endif

#define ULL_CHAR_MAX ((int) ((log(ULLONG_MAX)) / (log(10)) + 1))

/* ---------------------------------------------------------------------------*/

static MetricDefinition  metricDef[1];

static MetricRetriever   metricRetrBlockStorage;

#define DISKSTATS "/proc/diskstats"

/* ---------------------------------------------------------------------------*/

int _DefinedMetrics( MetricRegisterId *mr,
		     const char * pluginname,
		     size_t *mdnum,
		     MetricDefinition **md ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving metric definitions\n",
	  __FILE__,__LINE__);
#endif
  if (mr==NULL||mdnum==NULL||md==NULL) {
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
    return -1;
  }

  metricDef[0].mdVersion=MD_VERSION;
  metricDef[0].mdName="_BlockStorage";
  metricDef[0].mdReposPluginName="librepositoryStorage.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrBlockStorage;
  metricDef[0].mdeal=free;

  *mdnum=1;
  *md=metricDef;
  return 0;
}

int _StartStopMetrics (int starting) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : %s metric processing\n",
	  __FILE__,__LINE__,starting?"Starting":"Stopping");
#endif
  return 0;
}


/* ---------------------------------------------------------------------------*/
/* BlockStorage                                                               */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data BlockStorage has the following syntax :
 *
 * <read kb>:<write kb>:<capacity (in bytes)>:
 *
 */

int metricRetrBlockStorage( int mid, MetricReturner mret ) {  
  MetricValue * mv  = NULL;
  FILE * fhd        = NULL;
  char * ptr        = NULL;
  char * end        = NULL;
  char   blk[200];
  char   dev[210];
  char   buf[60000];
  char   values[(3*ULL_CHAR_MAX)+4];
  size_t bytes_read = 0;
  int    i          = 0;
  int   fd	    = 0;
  unsigned long long read, write, capacity = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving BlockStorage\n",
	  __FILE__,__LINE__);
#endif  
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {

#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric BlockStorage ID %d\n",
	    __FILE__,__LINE__,mid); 
#endif    

    if( (fhd = fopen(DISKSTATS,"r")) != NULL ) {
      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */
      if( bytes_read > 0 ) {
	  
	ptr = buf;
	while( (end = strchr(ptr,'\n')) != NULL ) {
	  sscanf(ptr,
		 " %*s %*s %[a-z0-9] %*s %*s %lld %*s %*s %*s %lld",
		 blk, &read, &write);
	
	  sprintf(dev, "/dev/%s", blk);
	  
	  fd = open(dev, O_RDONLY | O_NONBLOCK);
          if (fd > -1) {
	  	ioctl(fd, BLKGETSIZE64, &capacity);
	  }
	  close(fd);
	  
	  read = read / 2;   /* convert form sectors to kb */
	  write = write / 2; /* 512 bytes/sector, 1kb/1024 bytes = 1kb/2 sectors */
	  
	  memset(values,0,sizeof(values));
	  sprintf(values,"%lld:%lld:%lld:",
		  read, write, capacity);
	  
	  mv = calloc(1, sizeof(MetricValue) + 
		      (strlen(values)+1) +
		      (strlen(dev)+1) );
	  if (mv) {
	    mv->mvId = mid;
	    mv->mvTimeStamp = time(NULL);
	    mv->mvDataType = MD_STRING;
	    mv->mvDataLength = (strlen(values)+1);
	    mv->mvData = (char*)mv + sizeof(MetricValue);
	    strcpy(mv->mvData,values);
	    mv->mvResource = (char*)mv + sizeof(MetricValue) + (strlen(values)+1);
	    strcpy(mv->mvResource,dev);
	    mret(mv);
	  } 
	  ptr = end+1;
	  i++;
	}
       	fclose(fhd);
      }
      else { return -1; }
    }
    return i;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/*                      end of metricStorage.c                     */
/* ---------------------------------------------------------------------------*/
