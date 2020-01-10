/*
 * $Id: metricIPProtocolEndpoint.c,v 1.5 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2004, 2009
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
 * Metrics Gatherer Plugin of the following IP Protocol Endpoint specific 
 * metrics :
 *
 * BytesSubmitted
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <time.h>

/* ---------------------------------------------------------------------------*/

static MetricDefinition  metricDef[1];

/* --- BytesSubmitted is base for :
 * BytesTransmitted, BytesReceived, ErrorRate,
 * PacketsTransmitted, PacketsReceived
 * --- */
static MetricRetriever   metricRetrBytesSubmitted;

/* ---------------------------------------------------------------------------*/

#define NETINFO "/proc/net/dev"

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
  metricDef[0].mdName="BytesSubmitted";
  metricDef[0].mdReposPluginName="librepositoryIPProtocolEndpoint.so";
  metricDef[0].mdId=mr(pluginname,metricDef[0].mdName);
  metricDef[0].mdSampleInterval=60;
  metricDef[0].mproc=metricRetrBytesSubmitted;
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
/* BytesSubmitted                                                             */
/* ---------------------------------------------------------------------------*/

/* 
 * The raw data BytesSubmitted has the following syntax :
 *
 * <bytes received>:<bytes transmitted>:
 * <errors received>:<errors transmitted>:
 * <packtes received>:<packets transmitted>
 *
 */

int metricRetrBytesSubmitted( int mid, 
			      MetricReturner mret ) {  
  MetricValue * mv  = NULL;
  FILE * fhd        = NULL;
  char * ptr        = NULL;
  char * end        = NULL;
  char   port[64];
  char   buf[60000];
  char   values[(6*sizeof(unsigned long long))+6];
  size_t bytes_read = 0;
  int    i          = 0;
  unsigned long long receive_byte, receive_packets, receive_error = 0;
  unsigned long long trans_byte, trans_packets, trans_error       = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving BytesSubmitted\n",
	  __FILE__,__LINE__);
#endif  
  if (mret==NULL) { fprintf(stderr,"Returner pointer is NULL\n"); }
  else {

#ifdef DEBUG
    fprintf(stderr,"--- %s(%i) : Sampling for metric BytesSubmitted ID %d\n",
	    __FILE__,__LINE__,mid); 
#endif    

    if( (fhd = fopen(NETINFO,"r")) != NULL ) {
      bytes_read = fread(buf, 1, sizeof(buf)-1, fhd);
      buf[bytes_read] = 0; /* safeguard end of buffer */
      if( bytes_read > 0 ) {
	/* skip first two lines */
	ptr = strchr(buf,'\n')+1;
	ptr = strchr(ptr,'\n')+1;
	  
	while( (end = strchr(ptr,'\n')) != NULL ) {
	  sscanf(ptr,
		 " %[^:]: %lld %lld %lld %*s %*s %*s %*s %*s %lld %lld %lld",
		 port,
		 &receive_byte,&receive_packets,&receive_error,
		 &trans_byte,&trans_packets,&trans_error);
	  
	  /*	  fprintf(stderr,"[%i] port: %s %lld %lld\n",i,port,receive_byte,receive_packets);*/
	  
	  memset(values,0,sizeof(values));
	  sprintf(values,"%lld:%lld:%lld:%lld:%lld:%lld",
		  receive_byte,trans_byte,receive_error,trans_error,receive_packets,trans_packets);
	  
	  mv = calloc(1, sizeof(MetricValue) + 
		      (strlen(values)+1) +
		      strlen("IPv4_") +
		      (strlen(port)+1) );
	  if (mv) {
	    mv->mvId = mid;
	    mv->mvTimeStamp = time(NULL);
	    mv->mvDataType = MD_STRING;
	    mv->mvDataLength = (strlen(values)+1);
	    mv->mvData = (char*)mv + sizeof(MetricValue);
	    strcpy(mv->mvData,values);
	    mv->mvResource = (char*)mv + sizeof(MetricValue) + (strlen(values)+1);
	    strcpy(mv->mvResource,"IPv4_");
	    strcat(mv->mvResource,port);
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
/*                      end of metricIPProtocolEndpoint.c                     */
/* ---------------------------------------------------------------------------*/
