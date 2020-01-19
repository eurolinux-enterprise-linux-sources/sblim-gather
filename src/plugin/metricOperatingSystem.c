/*
 * $Id: metricOperatingSystem.c,v 1.21 2012/10/01 02:15:36 tyreld Exp $
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
 * Metrics Gatherer Plugin of the following Operating System specific metrics :
 *
 * NumberOfUsers
 * NumberOfProcesses
 * CPUTime
 * MemorySize
 * PageInCounter
 * PageOutCounter
 * LoadCounter
 * ContextSwitchCounter
 * HardwareInterruptCounter
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
#include <sys/utsname.h>
#include <netinet/in.h>
#include <limits.h>
#include <math.h>

#ifndef ULLONG_MAX
#   define ULLONG_MAX 18446744073709551615ULL
#endif

#define ULL_CHAR_MAX ((int) ((log(ULLONG_MAX)) / (log(10)) + 1))

/* ---------------------------------------------------------------------------*/

#define KERNELRELEASE(maj,min,patch) ((maj) * 10000 + (min)*1000 + (patch))
static int kernel_release();

/* ---------------------------------------------------------------------------*/

static MetricDefinition  metricDef[9];

/* --- NumberOfUsers --- */
static MetricRetriever   metricRetrNumOfUser;

/* --- NumberOfProcesses --- */
static MetricRetriever   metricRetrNumOfProc;

/* --- CPUTime is base for :
 * KernelModeTime, UserModeTime, TotalCPUTime,
 * InternalViewKernelModePercentage, InternalViewUserModePercentage,
 * InternalViewIdlePercentage, InternalViewTotalCPUPercentage,
 * ExternalViewKernelModePercentage, ExternalViewUserModePercentage,
 * ExternalViewTotalCPUPercentage,
 * CPUConsumptionIndex 
 * --- */
static MetricRetriever   metricRetrCPUTime;

/* --- MemorySize is base for :
 * TotalVisibleMemorySize,  FreePhysicalMemory, 
 * SizeStoredInPagingFiles, FreeSpaceInPagingFiles,
 * TotalVirtualMemorySize,  FreeVirtualMemory --- */
static MetricRetriever   metricRetrMemorySize;

/* --- PageInCounter, PageInRate --- */
static MetricRetriever   metricRetrPageInCounter;

/* --- PageOutCounter, PageOutRate --- */
static MetricRetriever   metricRetrPageOutCounter;

/* --- LoadCounter, LoadAverage --- */
static MetricRetriever   metricRetrLoadCounter;

/* --- ContextSwitchCounter, ContextSwitchRate --- */
static MetricRetriever   metricRetrContextSwitchCounter;

/* --- HardwareInterruptCounter, HardwareInterruptRate --- */
static MetricRetriever   metricRetrHardwareInterruptCounter;

/* ---------------------------------------------------------------------------*/

static char * resource = "OperatingSystem";

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
  metricDef[0].mdName="NumberOfUsers";
  metricDef[0].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrNumOfUser;
  metricDef[0].mdeal=free;

  metricDef[1].mdVersion=MD_VERSION;
  metricDef[1].mdName="NumberOfProcesses";
  metricDef[1].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[1].mdId=mr(pluginname,metricDef[1].mdName);
  metricDef[1].mdSampleInterval=60;
  metricDef[1].mproc=metricRetrNumOfProc;
  metricDef[1].mdeal=free;

  metricDef[2].mdVersion=MD_VERSION;
  metricDef[2].mdName="CPUTime";
  metricDef[2].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[2].mdId=mr(pluginname,metricDef[2].mdName);
  metricDef[2].mdSampleInterval=60;
  metricDef[2].mproc=metricRetrCPUTime;
  metricDef[2].mdeal=free;

  metricDef[3].mdVersion=MD_VERSION;
  metricDef[3].mdName="MemorySize";
  metricDef[3].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[3].mdId=mr(pluginname,metricDef[3].mdName);
  metricDef[3].mdSampleInterval=60;
  metricDef[3].mproc=metricRetrMemorySize;
  metricDef[3].mdeal=free;

  metricDef[4].mdVersion=MD_VERSION;
  metricDef[4].mdName="PageInCounter";
  metricDef[4].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[4].mdId=mr(pluginname,metricDef[4].mdName);
  metricDef[4].mdSampleInterval=60;
  metricDef[4].mproc=metricRetrPageInCounter;
  metricDef[4].mdeal=free;

  metricDef[5].mdVersion=MD_VERSION;
  metricDef[5].mdName="PageOutCounter";
  metricDef[5].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[5].mdId=mr(pluginname,metricDef[5].mdName);
  metricDef[5].mdSampleInterval=60;
  metricDef[5].mproc=metricRetrPageOutCounter;
  metricDef[5].mdeal=free;

  metricDef[6].mdVersion=MD_VERSION;
  metricDef[6].mdName="LoadCounter";
  metricDef[6].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[6].mdId=mr(pluginname,metricDef[6].mdName);
  metricDef[6].mdSampleInterval=60;
  metricDef[6].mproc=metricRetrLoadCounter;
  metricDef[6].mdeal=free;

  metricDef[7].mdVersion=MD_VERSION;
  metricDef[7].mdName="ContextSwitchCounter";
  metricDef[7].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[7].mdId=mr(pluginname,metricDef[7].mdName);
  metricDef[7].mdSampleInterval=60;
  metricDef[7].mproc=metricRetrContextSwitchCounter;
  metricDef[7].mdeal=free;

  metricDef[8].mdVersion=MD_VERSION;
  metricDef[8].mdName="HardwareInterruptCounter";
  metricDef[8].mdReposPluginName="librepositoryOperatingSystem.so";
  metricDef[8].mdId=mr(pluginname,metricDef[8].mdName);
  metricDef[8].mdSampleInterval=60;
  metricDef[8].mproc=metricRetrHardwareInterruptCounter;
  metricDef[8].mdeal=free;

  *mdnum=9;
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
/* NumberOfUsers                                                              */
/* ---------------------------------------------------------------------------*/

int metricRetrNumOfUser( int mid, 
			 MetricReturner mret ) { 
  MetricValue * mv  = NULL;  
  char          str[255];
  int           fd_out[2]; 
  int           fd_stdout;
  int           fd_err[2]; 
  int           fd_stderr;
  int           rc  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving NumberOfUsers\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric NumberOfUsers ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    if( pipe(fd_out)==0 && pipe(fd_err)==0 ) {
      memset(str,0,sizeof(str));

      fd_stdout = dup( fileno(stdout) );
      dup2( fd_out[1], fileno(stdout) );

      fd_stderr = dup( fileno(stderr) );
      dup2( fd_err[1], fileno(stderr) );

      rc = system("who -u | wc -l");
      if( rc == 0 ) { read( fd_out[0], str, sizeof(str)-1 ); }
      else          { read( fd_err[0], str, sizeof(str)-1 ); }
	 
      close(fd_out[1]);
      dup2( fd_stdout, fileno(stdout) );
      close(fd_out[0]);
      close(fd_stdout);

      close(fd_err[1]);
      dup2( fd_stderr, fileno(stderr) );
      close(fd_err[0]);
      close(fd_stderr);
    }
    else { return -1; }
      
    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT32;
      mv->mvDataLength = sizeof(unsigned);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned*)mv->mvData = htonl(atol(str));
      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
    
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* NumberOfProcesses                                                          */
/* ---------------------------------------------------------------------------*/

int metricRetrNumOfProc( int mid, 
			 MetricReturner mret ) {
  MetricValue   * mv    = NULL;
  int fd_out[2];
  int fd_err[2];
  int fd_stdout;
  int fd_stderr;
  char str[255];
  int rc = 0;


#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving NumberOfProcesses\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric NumberOfProcesses ID %d\n",
	    __FILE__,__LINE__,mid);
#endif

    /* get number of processes */
	if (pipe(fd_out)==0 && pipe(fd_err)==0) {
		memset(str, 0, sizeof(str));

        fd_stdout = dup( fileno(stdout) );
        dup2( fd_out[1], fileno(stdout) );

        fd_stderr = dup( fileno(stderr) );
        dup2( fd_err[1], fileno(stderr) );

        rc = system("ps -ef | wc -l");
        if (rc == 0) { read( fd_out[0], str, sizeof(str) - 1); }
        else         { read( fd_err[0], str, sizeof(str) - 1); }

        close(fd_out[1]);
        dup2( fd_stdout, fileno(stdout) );
        close(fd_out[0]);
        close(fd_stdout);

        close(fd_err[1]);
        dup2( fd_stderr, fileno(stderr) );
        close(fd_err[0]);
        close(fd_stderr);
    }
    else { return -1; }

    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT32;
      mv->mvDataLength = sizeof(unsigned);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned*)mv->mvData = htonl(atol(str));
      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* CPUTime                                                                    */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data CPUTime has the following syntax :
 *
 * <user mode>:<user mode with low priority(nice)>:<system mode>:<idle task>:
 * <iowait>:<irq>:<softirq>:<steal time>:<guest time>
 *
 * the values in CPUTime are saved in Jiffies ( 1/100ths of a second )
 */

int metricRetrCPUTime( int mid, 
		       MetricReturner mret ) {
  MetricValue * mv  = NULL;
  FILE * fhd        = NULL;
  char   buf[30000];
  char * ptr        = NULL;
  char * end        = NULL;
  char * hlp        = NULL;
  size_t bytes_read = 0;
  int    i          = 0;

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
    if( (fhd = fopen("/proc/stat","r")) != NULL ) {

      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */
      ptr = strstr(buf,"cpu")+3;
      while( *ptr == ' ') { ptr++; }
      end = strchr(ptr, '\n');

      /* replace ' ' with ':' */
      hlp = ptr;
      for( ; i<8; i++ ) {
	hlp = strchr(hlp, ' ');
	*hlp = ':';
      }
      fclose(fhd);
    }
    else { return -1; }

    mv = calloc( 1, sizeof(MetricValue) + 
		    (strlen(ptr)-strlen(end)+1)  + 
		    (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_STRING;
      mv->mvDataLength = (strlen(ptr)-strlen(end)+1);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      strncpy( mv->mvData, ptr, (strlen(ptr)-strlen(end)) );
      mv->mvResource = (char*)mv + sizeof(MetricValue) + ((strlen(ptr)-strlen(end))+1);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* MemorySize                                                                 */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data MemorySize has the following syntax :
 *
 * <TotalVisibleMemorySize>:<FreePhysicalMemory>:<SizeStoredInPagingFiles>:<FreeSpaceInPagingFiles>
 *
 * the values in MemorySize are saved in kBytes
 */

int metricRetrMemorySize( int mid, 
			  MetricReturner mret ) {
  MetricValue        * mv         = NULL;
  unsigned long long totalPhysMem = 0;
  unsigned long long freePhysMem  = 0;
  unsigned long long totalSwapMem = 0;
  unsigned long long freeSwapMem  = 0;
  FILE               * fhd        = NULL;
  char               * ptr        = NULL;
  char               * str        = NULL;
  char               buf[30000];
  size_t             bytes_read   = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving MemorySize\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric MemorySize ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    if ( (fhd=fopen("/proc/meminfo","r")) != NULL ) {

      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */

      ptr = strstr(buf,"MemTotal");
      sscanf(ptr, "%*s %lld", &totalPhysMem);
      ptr = strstr(buf,"MemFree");
      sscanf(ptr, "%*s %lld", &freePhysMem);
      ptr = strstr(buf,"SwapTotal");
      sscanf(ptr, "%*s %lld", &totalSwapMem);
      ptr = strstr(buf,"SwapFree");
      sscanf(ptr, "%*s %lld", &freeSwapMem);

      fclose(fhd);
    }
    else { return -1; }

    str = calloc(1, ((4*ULL_CHAR_MAX)+4) );
    sprintf( str,"%lld:%lld:%lld:%lld",
	     totalPhysMem,freePhysMem,totalSwapMem,freeSwapMem);
    
    mv = calloc( 1, sizeof(MetricValue) + 
		    (strlen(str)+1)  + 
		    (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_STRING;
      mv->mvDataLength = strlen(str)+1;
      mv->mvData = (char*)mv + sizeof(MetricValue);
      strcpy( mv->mvData, str );
      mv->mvResource = (char*)mv + sizeof(MetricValue) + strlen(str) +1;
      strcpy(mv->mvResource,resource);
      mret(mv);
    }

    if(str) free(str);
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageInCounter                                                              */
/* ---------------------------------------------------------------------------*/

int metricRetrPageInCounter( int mid, 
			     MetricReturner mret ) { 
  MetricValue * mv  = NULL; 
  FILE        * fhd = NULL;
  char          buf[30000];
  char        * ptr = NULL;
  size_t bytes_read = 0;
  unsigned long long in = 0;

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

    /* kernel 2.4 or lower */
    if(kernel_release() < 25000 ) {
      if ( (fhd=fopen("/proc/stat","r")) != NULL ) {
	bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
	buf[bytes_read] = 0; /* safeguard end of buffer */
	ptr = strstr(buf,"swap");
	sscanf(ptr, "%*s %lld", &in);
	fclose(fhd);
      }
      else { return -1; }
    }
    /* kernel 2.5 or higher */
    else {
      if ( (fhd=fopen("/proc/vmstat","r")) != NULL ) {
	bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
	buf[bytes_read] = 0; /* safeguard end of buffer */
	ptr = strstr(buf,"pswpin");
	sscanf(ptr, "%*s %lld", &in);
	fclose(fhd);
      }
      else { return -1; }
    }
    
    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned long long) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT64;
      mv->mvDataLength = sizeof(unsigned long long);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned long long*)mv->mvData = htonll(in);


      /*fprintf(stderr,"PageInCounter: origin %llu : converted %llu\n",in,*(unsigned long long*)mv->mvData);*/


      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
      
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PageOutCounter                                                             */
/* ---------------------------------------------------------------------------*/

int metricRetrPageOutCounter( int mid, 
			      MetricReturner mret ) { 
  MetricValue * mv  = NULL; 
  FILE        * fhd = NULL;
  char          buf[30000];
  char        * ptr = NULL;
  size_t bytes_read = 0;
  unsigned long long out = 0;

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

    /* kernel 2.4 or lower */
    if(kernel_release() < 25000 ) {
      if ( (fhd=fopen("/proc/stat","r")) != NULL ) {
	bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
	buf[bytes_read] = 0; /* safeguard end of buffer */
	ptr = strstr(buf,"swap");
	sscanf(ptr, "%*s %*s %lld", &out);
	fclose(fhd);
      }
      else { return -1; }
    }
    /* kernel 2.5 or higher */
    else {
      if ( (fhd=fopen("/proc/vmstat","r")) != NULL ) {
	bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
	buf[bytes_read] = 0; /* safeguard end of buffer */
	ptr = strstr(buf,"pswpout");
	sscanf(ptr, "%*s %lld", &out);
	fclose(fhd);
      }
      else { return -1; }
    }

    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned long long) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT64;
      mv->mvDataLength = sizeof(unsigned long long);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned long long*)mv->mvData = htonll(out);
      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
      
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* LoadCounter                                                                */
/* ---------------------------------------------------------------------------*/

int metricRetrLoadCounter( int mid, 
			   MetricReturner mret ) { 
  MetricValue * mv   = NULL; 
  FILE        * fhd  = NULL;
  float         load = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving LoadCounter\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric LoadCounter ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    if ( (fhd=fopen("/proc/loadavg","r")) != NULL ) {
      fscanf(fhd, 
	     "%f",
	     &load);
      fclose(fhd);
    }
    else { return -1; }

    mv = calloc(1, sizeof(MetricValue) +
		   sizeof(float) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_FLOAT32;
      mv->mvDataLength = sizeof(float);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(float*)mv->mvData = htonf(load);


      /*fprintf(stderr,"LoadCounter: origin %f : converted %f\n",load,*(float*)mv->mvData);*/


      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(float);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ContextSwitchCounter                                                       */
/* ---------------------------------------------------------------------------*/

int metricRetrContextSwitchCounter( int mid, 
				    MetricReturner mret ) { 
  MetricValue * mv  = NULL; 
  FILE        * fhd = NULL;
  char          buf[30000];
  char        * ptr = NULL;
  size_t bytes_read = 0;
  unsigned long long ctxt = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving ContextSwitchCounter\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric ContextSwitchCounter ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    if ( (fhd=fopen("/proc/stat","r")) != NULL ) {
      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */
      ptr = strstr(buf,"ctxt");
      sscanf(ptr, "%*s %lld", &ctxt);
      fclose(fhd);
    }
    else { return -1; }

    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned long long) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT64;
      mv->mvDataLength = sizeof(unsigned long long);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned long long*)mv->mvData = htonll(ctxt);
      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
      
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* HardwareInterruptCounter                                                   */
/* ---------------------------------------------------------------------------*/

int metricRetrHardwareInterruptCounter( int mid, 
					MetricReturner mret ) { 
  MetricValue * mv  = NULL; 
  FILE        * fhd = NULL;
  char          buf[30000];
  char        * ptr = NULL;
  size_t bytes_read = 0;
  unsigned long long intr = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving HardwareInterruptCounter\n",
	  __FILE__,__LINE__);
#endif
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {
#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric HardwareInterruptCounter ID %d\n",
	    __FILE__,__LINE__,mid);
#endif
    if ( (fhd=fopen("/proc/stat","r")) != NULL ) {
      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */
      ptr = strstr(buf,"intr");
      sscanf(ptr, "%*s %lld", &intr);
      fclose(fhd);
    }
    else { return -1; }

    mv = calloc(1, sizeof(MetricValue) + 
		   sizeof(unsigned long long) + 
		   (strlen(resource)+1) );
    if (mv) {
      mv->mvId = mid;
      mv->mvTimeStamp = time(NULL);
      mv->mvDataType = MD_UINT64;
      mv->mvDataLength = sizeof(unsigned long long);
      mv->mvData = (char*)mv + sizeof(MetricValue);
      *(unsigned long long*)mv->mvData = htonll(intr);
      mv->mvResource = (char*)mv + sizeof(MetricValue) + sizeof(unsigned long long);
      strcpy(mv->mvResource,resource);
      mret(mv);
    }
      
    return 1;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* get Kernel Version                                                         */
/* ---------------------------------------------------------------------------*/

int kernel_release() {

  struct utsname uts;
  int major, minor, patch;

  if (uname(&uts) < 0)
    return -1;
  if (sscanf(uts.release, "%d.%d.%d", &major, &minor, &patch) != 3)
    return -1;
  return KERNELRELEASE(major, minor, patch);

}


/* ---------------------------------------------------------------------------*/
/*                    end of metricOperatingSystem.c                          */
/* ---------------------------------------------------------------------------*/
