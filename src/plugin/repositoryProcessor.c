/*
 * $Id: repositoryProcessor.c,v 1.7 2012/07/31 23:26:46 tyreld Exp $
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
 * Repository Plugin of the following Processor specific metrics :
 *
 * TotalCPUTimePercentage
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commutil.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[2];

/* --- TotalCPUTimePercentage --- */
static MetricCalculator  metricCalcCPUTimePerc;

/* --- CPUTime --- */
static MetricCalculator  metricCalcCPUTime;

/* unit definitions */
static char * muPercent = "Percent";
static char * muNa = "N/A";

struct cpu_times {
   unsigned long long ut, un, kt, it;
};

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
  metricCalcDef[0].mcName="CPUTime";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcCPUTime;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="TotalCPUTimePercentage";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[1].mcChangeType=MD_GAUGE;
  metricCalcDef[1].mcIsContinuous=MD_TRUE;
  metricCalcDef[1].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[1].mcDataType=MD_FLOAT32;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalcCPUTimePerc;
  metricCalcDef[1].mcUnits=muPercent;
   
  *mcnum=2;
  *mc=metricCalcDef;
  return 0;
}

/* ---------------------------------------------------------------------------*/
/* CPUTime                                                                    */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCPUTime(MetricValue *mv, int mnum, void *v, size_t vlen) {

#ifdef DEBUG
   fprintf(stderr,"--- %s(%i) : Calculate CPUTime\n",
         __FILE__,__LINE__);
#endif

   if (mv && (vlen >= mv->mvDataLength) && (mnum==1)) {
      memcpy(v, mv->mvData, mv->mvDataLength);
      return mv->mvDataLength;
   }
   return -1;
}

/* ---------------------------------------------------------------------------*/
/* TotalCPUTimePercentage                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcCPUTimePerc(MetricValue *mv, int mnum, void *v, size_t vlen) {

   struct cpu_times times[2];
   float stotal = 0;
   float etotal = 0;
   float scale = 0;
   unsigned long long idle = 0;
   float tp;

#ifdef DEBUG
   fprintf(stderr,"--- %s(%i) : Calculate TotalCPUTimePercentage\n",
         __FILE__,__LINE__);
#endif

   if (mv && (vlen >= sizeof(float)) && (mnum >= 1)) {
      sscanf(mv[0].mvData, "cpu%*d:%Lu:%Lu:%Lu:%Lu", &times[0].ut, 
            &times[0].un, &times[0].kt, &times[0].it);
      etotal = (float) (times[0].ut + times[0].un + times[0].kt + times[0].it);

      if (mnum > 1) {
         sscanf(mv[mnum - 1].mvData, "cpu%*d:%Lu:%Lu:%Lu:%Lu", &times[1].ut,
               &times[1].un, &times[1].kt, &times[1].it);
         
         stotal = (float) (times[1].ut + times[1].un + times[1].kt + times[1].it);
      }

      scale = 100.0 / (float) (etotal - stotal);
      idle = times[0].it - times[1].it;
      
      tp = 100.0 - ((float) idle * scale);

      memcpy(v, &tp, sizeof(float));
      return sizeof(float);
   }

   return -1;
}

/* ---------------------------------------------------------------------------*/
/*                         end of repositoryProcessor.c                       */
/* ---------------------------------------------------------------------------*/
