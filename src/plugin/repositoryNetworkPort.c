/*
 * $Id: repositoryNetworkPort.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 * Repository Plugin of the following Network Port specific metrics :
 *
 * BytesSubmitted
 * BytesTransmitted
 * BytesReceived
 * ErrorRate
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[4];

/* --- BytesSubmitted is base for :
 * BytesTransmitted, BytesReceived, ErrorRate --- */
static MetricCalculator  metricCalcBytesSubmitted;
static MetricCalculator  metricCalcBytesTransmitted;
static MetricCalculator  metricCalcBytesReceived;
static MetricCalculator  metricCalcErrorRate;


/* unit definitions */
static char * muBytes = "Bytes";
static char * muErrorsPerSecond = "Errors per second";
static char * muNa ="N/A";

/* ---------------------------------------------------------------------------*/

#ifdef NOTNOW
static unsigned long long ip_getPacketsReceived( char * data );
static unsigned long long ip_getPacketsTransmitted( char * data );
#endif
static unsigned long long ip_getBytesTransmitted( char * data );
static unsigned long long ip_getBytesReceived( char * data );
static unsigned long long ip_getErrorsTransmitted( char * data );
static unsigned long long ip_getErrorsReceived( char * data );

/* ---------------------------------------------------------------------------*/

int _DefinedRepositoryMetrics( MetricRegisterId *mr,
			       const char *pluginname,
			       size_t *mcnum,
			       MetricCalculationDefinition **mc ) {
#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Retrieving metric calculation definitions\n",
	  __FILE__,__LINE__);
#endif
  if (mr==NULL||mcnum==NULL||mc==NULL) {
    fprintf(stderr,"--- %s(%i) : invalid parameter list\n",__FILE__,__LINE__);
    return -1;
  }

  metricCalcDef[0].mcVersion=MD_VERSION;
  metricCalcDef[0].mcName="BytesSubmitted";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcBytesSubmitted;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="BytesTransmitted";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_SUMMABLE;
  metricCalcDef[1].mcDataType=MD_UINT64;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcBytesTransmitted;
  metricCalcDef[1].mcUnits=muBytes;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="BytesReceived";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_SUMMABLE;
  metricCalcDef[2].mcDataType=MD_UINT64;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcBytesReceived;
  metricCalcDef[2].mcUnits=muBytes;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="ErrorRate";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[3].mcDataType=MD_FLOAT32;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcErrorRate;
  metricCalcDef[3].mcUnits=muErrorsPerSecond;

  *mcnum=4;
  *mc=metricCalcDef;
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

size_t metricCalcBytesSubmitted( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BytesSubmitted\n",
	  __FILE__,__LINE__);
#endif
  /* plain copy */
  if (mv && (vlen>=mv->mvDataLength) && (mnum==1) ) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    return mv->mvDataLength;
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* BytesTransmitted                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcBytesTransmitted( MetricValue *mv,   
				   int mnum,
				   void *v, 
				   size_t vlen ) {
  unsigned long long bt = 0;
  unsigned long long b1 = 0;
  unsigned long long b2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BytesTransmitted\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    b1 = ip_getBytesTransmitted(mv[0].mvData);
    if( mnum > 1 ) {
      b2 = ip_getBytesTransmitted(mv[mnum-1].mvData);
      bt = b1-b2;
    }
    else { bt = b1; }

    //fprintf(stderr,"bytes transmitted: %lld\n",bt);
    memcpy(v,&bt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* BytesReceived                                                              */
/* ---------------------------------------------------------------------------*/

size_t metricCalcBytesReceived( MetricValue *mv,   
				int mnum,
				void *v, 
				size_t vlen ) {
  unsigned long long br = 0;
  unsigned long long b1 = 0;
  unsigned long long b2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BytesReceived\n",
	  __FILE__,__LINE__);
#endif
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    b1 = ip_getBytesReceived(mv[0].mvData);
    if( mnum > 1 ) {
      b2 = ip_getBytesReceived(mv[mnum-1].mvData);
      br = b1-b2;
    }
    else { br = b1; }

    //fprintf(stderr,"bytes received: %lld\n",br);
    memcpy(v,&br,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ErrorRate                                                                  */
/* ---------------------------------------------------------------------------*/

size_t metricCalcErrorRate( MetricValue *mv,   
			    int mnum,
			    void *v, 
			    size_t vlen ) {
  float rate = 0;
  float et1  = 0;
  float et2  = 0;
  float er1  = 0;
  float er2  = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate ErrorRate\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(float)) && (mnum>=2) ) {

    et1 = ip_getErrorsTransmitted(mv[0].mvData);
    er1 = ip_getErrorsReceived(mv[0].mvData);

    et2 = ip_getErrorsTransmitted(mv[mnum-1].mvData);
    er2 = ip_getErrorsReceived(mv[mnum-1].mvData);

    rate = ( (et1+er1) - (et2+er2) ) /
           (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);

    //fprintf(stderr,"error rate: %f\n",rate);
    memcpy(v,&rate,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* tool functions on BytesSubmitted                                           */
/* ---------------------------------------------------------------------------*/

unsigned long long ip_getBytesReceived( char * data ) {

  char * hlp = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    memset(bytes,0,sizeof(bytes));
    strncpy(bytes, data, (strlen(data)-strlen(hlp)) );
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}


unsigned long long ip_getBytesTransmitted( char * data ) {

  char * hlp = NULL;
  char * end = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    end = strchr(hlp, ':');
    memset(bytes,0,sizeof(bytes));
    strncpy(bytes, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}


unsigned long long ip_getErrorsReceived( char * data ) {
  char * hlp = NULL;
  char * end = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(bytes,0,sizeof(bytes));
    strncpy(bytes, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}


unsigned long long ip_getErrorsTransmitted( char * data ) {
  char * hlp = NULL;
  char * end = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(bytes,0,sizeof(bytes));
    strncpy(bytes, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}

#ifdef NOTNOW

unsigned long long ip_getPacketsReceived( char * data ) {
  char * hlp = NULL;
  char * end = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strchr(data, ':')) != NULL ) {
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    hlp = strchr(hlp, ':');
    hlp++;
    end = strchr(hlp, ':');
    memset(bytes,0,sizeof(bytes));
    strncpy(bytes, hlp, (strlen(hlp)-strlen(end)) );
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}


unsigned long long ip_getPacketsTransmitted( char * data ) {
  char * hlp = NULL;
  char   bytes[128];
  unsigned long long val = 0;

  if( (hlp = strrchr(data, ':')) != NULL ) {
    hlp++;
    memset(bytes,0,sizeof(bytes));
    strcpy(bytes, hlp);
    val = strtoll(bytes,(char**)NULL,10);
  }

  return val;
}

#endif

/* ---------------------------------------------------------------------------*/
/*                        end of repositoryNetworkPort.c                      */
/* ---------------------------------------------------------------------------*/
