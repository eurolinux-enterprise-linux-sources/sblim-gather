/*
 * $Id: repositoryStorage.c,v 1.2 2012/07/31 23:26:46 tyreld Exp $
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
 * Repository Plugin of the following IP Protocol Endpoint specific metrics :
 *
 * _BlockStorage
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[4];

static MetricCalculator  metricCalcBlockStorage;
static MetricCalculator  metricCalcRead;
static MetricCalculator	 metricCalcWrite;
static MetricCalculator	 metricCalcCapacity;

/* unit definitions */
static char * muBytes = "Bytes";
static char * muKbytes = "Kilobytes";
static char * muNa ="N/A";

/* ---------------------------------------------------------------------------*/

enum indices { READ, WRITE, CAPACITY };
static unsigned long long dev_getData(char * data, char index);

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
  metricCalcDef[0].mcName="_BlockStorage";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcBlockStorage;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="Read";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_SUMMABLE;
  metricCalcDef[1].mcDataType=MD_UINT64;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcRead;
  metricCalcDef[1].mcUnits=muKbytes;

  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="Write";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[2].mcChangeType=MD_GAUGE;
  metricCalcDef[2].mcIsContinuous=MD_TRUE;
  metricCalcDef[2].mcCalculable=MD_SUMMABLE;
  metricCalcDef[2].mcDataType=MD_UINT64;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalcWrite;
  metricCalcDef[2].mcUnits=muKbytes;

  metricCalcDef[3].mcVersion=MD_VERSION;
  metricCalcDef[3].mcName="Capacity";
  metricCalcDef[3].mcId=mr(pluginname,metricCalcDef[3].mcName);
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[3].mcDataType=MD_UINT64;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcCapacity;
  metricCalcDef[3].mcUnits=muBytes;

  *mcnum=4;
  *mc=metricCalcDef;
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

size_t metricCalcBlockStorage( MetricValue *mv,   
				 int mnum,
				 void *v, 
				 size_t vlen ) {

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate BlockStorage\n",
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
/* Read                                                                       */
/* ---------------------------------------------------------------------------*/

size_t metricCalcRead( MetricValue *mv,   
				   int mnum,
				   void *v, 
				   size_t vlen ) {
  unsigned long long rt = 0;
  unsigned long long r1 = 0;
  unsigned long long r2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate Read\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    r1 = dev_getData(mv[0].mvData, READ);
    if( mnum > 1 ) {
      r2 = dev_getData(mv[mnum-1].mvData, READ);
      rt = r1-r2;
    }
    else { rt = r1; }

    memcpy(v,&rt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* Write                                                                      */
/* ---------------------------------------------------------------------------*/

size_t metricCalcWrite( MetricValue *mv,   
				   int mnum,
				   void *v, 
				   size_t vlen ) {
  unsigned long long wt = 0;
  unsigned long long w1 = 0;
  unsigned long long w2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate Write\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    w1 = dev_getData(mv[0].mvData, WRITE);
    if( mnum > 1 ) {
      w2 = dev_getData(mv[mnum-1].mvData, WRITE);
      wt = w1-w2;
    }
    else { wt = w1; }

    memcpy(v,&wt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* Capacity                                                                   */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCapacity( MetricValue *mv,   
				   int mnum,
				   void *v, 
				   size_t vlen ) {
  unsigned long long ct = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate Capacity\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    ct = dev_getData(mv[0].mvData, CAPACITY);

    memcpy(v,&ct,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* tool functions on BlockStorage                                             */
/* ---------------------------------------------------------------------------*/

unsigned long long dev_getData(char * data, char index) {
	char * hlp = NULL;
	char * end = NULL;
	char bytes[128];
	unsigned long long val = 0;
	int i = 0;

	hlp = data;
	if ((end = strchr(hlp, ':')) != NULL) {
		for (i = 0; i < index; i++) {
			hlp = ++end;
			if ((end = strchr(hlp, ':')) == NULL) {
				return val;
			}
		}

		memset(bytes, 0, sizeof(bytes));
		strncpy(bytes, hlp, strlen(hlp) - strlen(end));
		val = strtoll(bytes, (char**) NULL, 10);
	}

	return val;
}


/* ---------------------------------------------------------------------------*/
/*                        end of repositoryStorage.c                          */
/* ---------------------------------------------------------------------------*/
