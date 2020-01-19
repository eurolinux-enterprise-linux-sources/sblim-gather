/*
 * $Id: repositorySample4.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: 
 *
 * Description: Sample Code
 */

#include <rplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MetricCalculationDefinition metricCalcDef[3];

static MetricCalculator  metricCalculator1;
static MetricCalculator  metricCalculator2;
static MetricCalculator  metricCalculator3;

int _DefinedRepositoryMetrics (MetricRegisterId *mr,
			       const char *pluginname,
			       size_t *mcnum,
			       MetricCalculationDefinition **mc)
{
#ifdef DEBUG
  fprintf(stderr,"Retrieving metric calculation definitions\n");
#endif
  if (mr==NULL||mcnum==NULL||mc==NULL) {
    fprintf(stderr,"invalid parameter list\n");
    return -1;
  }
  metricCalcDef[0].mcVersion=MD_VERSION;
  metricCalcDef[0].mcName="sample4-1";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_RETRIEVED | MD_POINT;
  metricCalcDef[0].mcDataType=MD_SINT32;
  metricCalcDef[0].mcCalc=metricCalculator1;
  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="sample4-2";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_CALCULATED | MD_INTERVAL;
  metricCalcDef[1].mcDataType=MD_SINT32;
  metricCalcDef[1].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[1].mcCalc=metricCalculator2;
  metricCalcDef[2].mcVersion=MD_VERSION;
  metricCalcDef[2].mcName="sample4-3";
  metricCalcDef[2].mcId=mr(pluginname,metricCalcDef[2].mcName);
  metricCalcDef[2].mcMetricType=MD_CALCULATED | MD_RATE    ;
  metricCalcDef[2].mcDataType=MD_SINT32;
  metricCalcDef[2].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[2].mcCalc=metricCalculator3;
  *mcnum=3;
  *mc=metricCalcDef;
  return 0;
}

size_t metricCalculator1(MetricValue *mv, int mnum, void *v, size_t vlen)
{
  /* plain copy */
  if (mv && vlen >=  mv->mvDataLength) {
    memcpy(v,mv->mvData,mv->mvDataLength);
    return mv->mvDataLength;
  }
  return -1;
}

size_t metricCalculator2(MetricValue *mv, int mnum, void *v, size_t vlen)
{
  /* some calculation*/
  int i1, i2;
  if (mv && vlen >=  mv->mvDataLength) {
    i1 = *(int*)mv[0].mvData;
    if (mnum > 1) {
      i2 = *(int*)mv[mnum-1].mvData;
    } else {
      i2 = 0;
    }
    *(int*)v = i1 - i2;
    return sizeof(int);
  }
  return -1;
}

size_t metricCalculator3(MetricValue *mv, int mnum, void *v, size_t vlen)
{
  /* some calculation*/
  int i1, i2;
  if (mv && vlen >=  mv->mvDataLength &&
      strcmp(mv->mvResource,"sample4res") == 0) {
    i1 = *(int*)mv[0].mvData;
    if (mnum > 1) {
      i2 = *(int*)mv[mnum-1].mvData;
    } else {
      i2 = 0;
    }
    *(int*)v = i1 - i2;
    return sizeof(int);
  }
  return -1;
}
