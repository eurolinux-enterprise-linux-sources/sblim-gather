/*
 * $Id: metricUnixProcess.c,v 1.18 2011/12/23 01:53:45 tyreld Exp $
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
 * Author:       Heidi Neumann <heidineu@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * Metrics Gatherer Plugin of the following Unix Process specific metrics :
 *
 * CPUTime
 * ResidentSetSize
 * PageInCounter
 * PageOutCounter
 * VirtualSize
 * SharedSize
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <commutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

/* ---------------------------------------------------------------------------*/

static MetricDefinition  metricDef[6];

/* --- CPUTime is base for :
 * KernelModeTime, UserModeTime, TotalCPUTime,
 * InternalViewKernelModePercentage, InternalViewUserModePercentage,
 * InternalViewTotalCPUPercentage, ExternalViewKernelModePercentage,
 * ExternalViewUserModePercentage, ExternalViewTotalCPUPercentage,
 * AccumulatedKernelModeTime, AccumulatedUserModeTime,
 * AccumulatedTotalCPUTime
 * --- */
static MetricRetriever   metricRetrCPUTime;

/* --- ResidentSetSize --- */
static MetricRetriever   metricRetrResSetSize;

/* --- PageInCounter, PageInRate --- */
static MetricRetriever   metricRetrPageInCounter;

/* --- PageOutCounter, PageOutRate --- */
static MetricRetriever   metricRetrPageOutCounter;

/* --- VirtualSize --- */
static MetricRetriever   metricRetrVirtualSize;

/* --- SharedSize --- */
static MetricRetriever   metricRetrSharedSize;

/* ---------------------------------------------------------------------------*/

static int enum_all_pid( char ** list );

/* ---------------------------------------------------------------------------*/

int _DefinedMetrics ( MetricRegisterId *mr,
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
  metricDef[0].mdName="CPUTime";
  metricDef[0].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrCPUTime;
  metricDef[0].mdeal=free;

  metricDef[1].mdVersion=MD_VERSION;
  metricDef[1].mdName="ResidentSetSize";
  metricDef[1].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[1].mdId=mr(pluginname,metricDef[1].mdName);
  metricDef[1].mdSampleInterval=60;
  metricDef[1].mproc=metricRetrResSetSize;
  metricDef[1].mdeal=free;

  metricDef[2].mdVersion=MD_VERSION;
  metricDef[2].mdName="PageInCounter";
  metricDef[2].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[2].mdId=mr(pluginname,metricDef[2].mdName);
  metricDef[2].mdSampleInterval=60;
  metricDef[2].mproc=metricRetrPageInCounter;
  metricDef[2].mdeal=free;

  metricDef[3].mdVersion=MD_VERSION;
  metricDef[3].mdName="VirtualSize";
  metricDef[3].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[3].mdId=mr(pluginname,metricDef[3].mdName);
  metricDef[3].mdSampleInterval=60;
  metricDef[3].mproc=metricRetrVirtualSize;
  metricDef[3].mdeal=free;

  metricDef[4].mdVersion=MD_VERSION;
  metricDef[4].mdName="SharedSize";
  metricDef[4].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[4].mdId=mr(pluginname,metricDef[4].mdName);
  metricDef[4].mdSampleInterval=60;
  metricDef[4].mproc=metricRetrSharedSize;
  metricDef[4].mdeal=free;

  metricDef[5].mdVersion=MD_VERSION;
  metricDef[5].mdName="PageOutCounter";
  metricDef[5].mdReposPluginName="librepositoryUnixProcess.so";
  metricDef[5].mdId=mr(pluginname,metricDef[5].mdName);
  metricDef[5].mdSampleInterval=60;
  metricDef[5].mproc=metricRetrPageOutCounter;
  metricDef[5].mdeal=free;

  *mdnum=6;
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
/* CPUTime                                                                    */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data CPUTime has the following syntax :
 *
 * <PID user mode time>:<PID kernel mode time>:<OS user mode>:
 * <OS user mode with low priority(nice)>:<OS system mode>:<OS idle task>
 *
 * the values in CPUTime are saved in Jiffies ( 1/100ths of a second )
 */

int metricRetrCPUTime( int mid, 
		       MetricReturner mret ) {
  MetricValue * mv          = NULL;
  FILE        * fhd         = NULL;
  char        * _enum_pid   = NULL;
  char        * ptr         = NULL;
  char        * end         = NULL;
  char        * hlp         = NULL;
  char          buf[4096];
  char          os_buf[4096];
  int           _enum_size  = 0;
  int           i           = 0;
  size_t        bytes_read  = 0;
  unsigned long long u_time = 0;
  unsigned long long k_time = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving CPUTime\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric CPUTime ID %d\n",
	    __FILE__,__LINE__,mid);
#endif

    /* get OS specific CPUTime */
    if( (fhd = fopen("/proc/stat","r")) != NULL ) {
      bytes_read = fread(os_buf, 1, sizeof(os_buf)-1, fhd);
      os_buf[bytes_read] = 0; /* safeguard end of buffer */
      ptr = strstr(os_buf,"cpu")+3;
      while( *ptr == ' ') { ptr++; }
      end = strchr(ptr, '\n');
      hlp = ptr;
      for( ; i<3; i++ ) { 
	hlp = strchr(hlp, ' ');
	*hlp = ':';
      }
      fclose(fhd);
    }
    else { return -1; }
    fhd=NULL;

    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	u_time = 0;
	k_time = 0;
	    
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/stat");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lld %lld", 
		 &u_time,&k_time );
	  fclose(fhd);
	}

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%lld:%lld:",u_time,k_time);
	strncpy(buf+strlen(buf),ptr,strlen(ptr)-strlen(end));

	mv = calloc( 1, sizeof(MetricValue) + 
		     (strlen(buf)+1) +
		     (strlen(_enum_pid+(i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_STRING;
	  mv->mvDataLength = (strlen(buf)+1);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  strncpy( mv->mvData, buf, strlen(buf) );	
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + (strlen(buf)+1);
	  strcpy(mv->mvResource,_enum_pid+(i*64));
	  mret(mv);
	}
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ResidentSetSize                                                            */
/* ---------------------------------------------------------------------------*/

int metricRetrResSetSize( int mid, 
			  MetricReturner mret ) {
  MetricValue   * mv         = NULL;
  FILE          * fhd        = NULL;
  char          * _enum_pid  = NULL;
  char            buf[254];
  int             _enum_size = 0;
  int             i          = 0;
  unsigned long long size    = 0;
  unsigned long long rss     = 0;


#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving ResidentSetSize\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric ResidentSetSize ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	size = 0;
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/stat");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		 "%*s %*s %*s %*s %*s %*s %*s %*s %*s %lld %*s",
		 &rss);
	  fclose(fhd);
	  size = rss * sysconf(_SC_PAGESIZE);
	}
	
	mv = calloc( 1, sizeof(MetricValue) + 
		     sizeof(unsigned long long) +
		     (strlen(_enum_pid + (i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_UINT64;
	  mv->mvDataLength = sizeof(unsigned long long);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  *(unsigned long long *)mv->mvData = htonll(size);	
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
	  strcpy(mv->mvResource,_enum_pid + (i*64));
	  mret(mv);
	}
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageInCounter                                                              */
/* ---------------------------------------------------------------------------*/

int metricRetrPageInCounter( int mid, 
			     MetricReturner mret ) { 
  MetricValue      * mv         = NULL; 
  FILE             * fhd        = NULL;
  char             * _enum_pid  = NULL;
  char               buf[254];
  int                _enum_size = 0;
  int                i          = 0;
  unsigned long long page = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving PageInCounter\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric PageInCounter ID %d\n",
	    __FILE__,__LINE__,mid);
#endif

    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	page = 0;
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/stat");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,
		 "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lld",
		 &page);
	  fclose(fhd);
	}

	mv = calloc( 1, sizeof(MetricValue) + 
		     sizeof(unsigned long long) +
		     (strlen(_enum_pid + (i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_UINT64;
	  mv->mvDataLength = sizeof(unsigned long long);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  *(unsigned long long*)mv->mvData = htonll(page);	 
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
	  strcpy(mv->mvResource,_enum_pid + (i*64));
	  mret(mv);
	}	
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageOutCounter                                                             */
/* ---------------------------------------------------------------------------*/

int metricRetrPageOutCounter( int mid, 
			      MetricReturner mret ) { 
  MetricValue      * mv         = NULL; 
  FILE             * fhd        = NULL;
  char             * _enum_pid  = NULL;
  char               buf[254];
  int                _enum_size = 0;
  int                i          = 0;
  unsigned long long page = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving PageOutCounter\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric PageOutCounter ID %d\n",
	    __FILE__,__LINE__,mid);
#endif

    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	page = 0;
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/stat");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,
		 "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		 "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		 "%*s %*s %*s %*s %lld",
		 &page);
	  fclose(fhd);
	}

	mv = calloc( 1, sizeof(MetricValue) + 
		     sizeof(unsigned long long) +
		     (strlen(_enum_pid + (i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_UINT64;
	  mv->mvDataLength = sizeof(unsigned long long);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  *(unsigned long long*)mv->mvData = htonll(page);	 
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
	  strcpy(mv->mvResource,_enum_pid + (i*64));
	  mret(mv);
	}	
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* VirtualSize                                                                */
/* ---------------------------------------------------------------------------*/

int metricRetrVirtualSize( int mid, 
			   MetricReturner mret ) {
  MetricValue   * mv         = NULL;
  FILE          * fhd        = NULL;
  char          * _enum_pid  = NULL;
  char            buf[254];
  int             _enum_size = 0;
  int             i          = 0;
  unsigned long long size    = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving VirtualSize\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric VirtualSize ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	size = 0;
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/stat");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		 "%*s %*s %*s %*s %*s %*s %*s %*s %lld",
		 &size);
	  fclose(fhd);
	}
	
	mv = calloc( 1, sizeof(MetricValue) + 
		     sizeof(unsigned long long) +
		     (strlen(_enum_pid + (i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_UINT64;
	  mv->mvDataLength = sizeof(unsigned long long);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  *(unsigned long long *)mv->mvData = htonll(size);	
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
	  strcpy(mv->mvResource,_enum_pid + (i*64));
	  mret(mv);
	}
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* SharedSize                                                                 */
/* ---------------------------------------------------------------------------*/

int metricRetrSharedSize( int mid, 
			  MetricReturner mret ) {
  MetricValue   * mv         = NULL;
  FILE          * fhd        = NULL;
  char          * _enum_pid  = NULL;
  char            buf[254];
  int             _enum_size = 0;
  int             i          = 0;
  unsigned long long size    = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving SharedSize\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric SharedSize ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    /* get number of processes */
    _enum_size = enum_all_pid( &_enum_pid );
    if( _enum_size > 0 ) {
      for(i=0;i<_enum_size;i++) {

	size = 0;
	memset(buf,0,sizeof(buf));
	strcpy(buf,"/proc/");
	strcat(buf,_enum_pid + (i*64));
	strcat(buf,"/statm");
	if( (fhd = fopen(buf,"r")) != NULL ) {
	  fscanf(fhd,"%*s %*s %lld",
		 &size);
	  fclose(fhd);
	}

	size = size * sysconf(_SC_PAGESIZE);
	
	mv = calloc( 1, sizeof(MetricValue) + 
		     sizeof(unsigned long long) +
		     (strlen(_enum_pid + (i*64))+1) );
	if (mv) {
	  mv->mvId = mid;
	  mv->mvTimeStamp = time(NULL);
	  mv->mvDataType = MD_UINT64;
	  mv->mvDataLength = sizeof(unsigned long long);
	  mv->mvData = (char*)mv + sizeof(MetricValue);
	  *(unsigned long long *)mv->mvData = htonll(size);	
	  mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
	  strcpy(mv->mvResource,_enum_pid + (i*64));
	  mret(mv);
	}
      }
      if(_enum_pid) free(_enum_pid);
      return _enum_size;
    }
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* get all Process IDs                                                        */
/* ---------------------------------------------------------------------------*/

int enum_all_pid( char ** list ) {  

  struct dirent * entry = NULL;
  DIR           * dir   = NULL;
  char          * _enum_pid = NULL;
  int             _enum_size = 0;
  int             i     = 1;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : enum_all_pid()\n",__FILE__,__LINE__);
#endif
  /* get number of processes */
  if( ( dir = opendir("/proc") ) != NULL ) {
    while( ( entry = readdir(dir)) != NULL ) {

      if( strcasecmp(entry->d_name,"1") == 0 ) { 
	_enum_size = 1;
	_enum_pid  = calloc(_enum_size,64);
	strcpy(_enum_pid,entry->d_name);
	while( ( entry = readdir(dir)) != NULL ) {
	  if( strncmp(entry->d_name,".",1) != 0 ) {
	    if( i==_enum_size ) {
	      _enum_size++;
	      _enum_pid = realloc(_enum_pid,_enum_size*64);
	      memset((_enum_pid + (i*64)), 0, 64);
	    }
	    strcpy(_enum_pid + (i*64),entry->d_name);
	    i++;
	  }
	}
      }
    }
    closedir(dir);
    *list = _enum_pid;
    return _enum_size;
  }
  else { return -1; }
}


/* ---------------------------------------------------------------------------*/
/*                       end of metricUnixProcess.c                           */
/* ---------------------------------------------------------------------------*/
