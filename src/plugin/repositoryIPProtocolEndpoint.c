/*
 * $Id: repositoryIPProtocolEndpoint.c,v 1.11 2012/07/31 23:26:45 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2004, 2009, 2011
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Heidi Neumann <heidineu@de.ibm.com>
 * Contributors: Tyrel Datwyler <tyreld@us.ibm.com>
 *
 * Description:
 * Repository Plugin of the following IP Protocol Endpoint specific metrics :
 *
 * _BytesSubmitted
 * BytesTransmitted
 * BytesReceived
 * ErrorRate
 * PacketsTransmitted
 * PacketsReceived
 * PacketTransmitRate
 * PacketReceiveRate
 * ReceivePacketsDropped
 * TransmitPacketsDropped
 * ReceivePacketDropRate
 * TransmitPacketDropRate
 *
 */

/* ---------------------------------------------------------------------------*/

#include <mplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------*/

static MetricCalculationDefinition metricCalcDef[14];

/* --- BytesSubmitted is base for :
 * BytesTransmitted, BytesReceived, ErrorRate,
 * PacketsTransmitted, PacketsReceived
 * --- */
static MetricCalculator  metricCalcBytesSubmitted;
static MetricCalculator  metricCalcBytesTransmitted;
static MetricCalculator  metricCalcBytesReceived;
static MetricCalculator  metricCalcErrorRate;
static MetricCalculator  metricCalcPacketsTransmitted;
static MetricCalculator  metricCalcPacketsReceived;
static MetricCalculator  metricCalcPacketTransRate;
static MetricCalculator  metricCalcPacketRecRate;
static MetricCalculator  metricCalcByteTransRate;
static MetricCalculator  metricCalcByteRecRate;
static MetricCalculator  metricCalcReceiveDrop;
static MetricCalculator  metricCalcTransmitDrop;
static MetricCalculator  metricCalcRecDropRate;
static MetricCalculator  metricCalcTransDropRate;

/* unit definitions */
static char * muBytes = "Bytes";
static char * muBytesPerSecond = "Bytes per second";
static char * muPackets = "Packets";
static char * muPacketsPerSecond = "Packets per second";
static char * muErrorsPerSecond = "Errors per second";
static char * muNa ="N/A";

/* ---------------------------------------------------------------------------*/

enum indices { RX_BYTE, TX_BYTE, RX_ERRS, TX_ERRS, RX_PACK, TX_PACK, RX_DROP, TX_DROP };
static unsigned long long ip_getData(char * data, char index);

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
  metricCalcDef[0].mcName="_BytesSubmitted";
  metricCalcDef[0].mcId=mr(pluginname,metricCalcDef[0].mcName);
  metricCalcDef[0].mcMetricType=MD_PERIODIC|MD_RETRIEVED|MD_POINT|MD_ORGSBLIM;
  metricCalcDef[0].mcIsContinuous=MD_FALSE;
  metricCalcDef[0].mcCalculable=MD_NONCALCULABLE;
  metricCalcDef[0].mcDataType=MD_STRING;
  metricCalcDef[0].mcCalc=metricCalcBytesSubmitted;
  metricCalcDef[0].mcUnits=muNa;

  metricCalcDef[1].mcVersion=MD_VERSION;
  metricCalcDef[1].mcName="BytesTransmitted";
  metricCalcDef[1].mcId=mr(pluginname,metricCalcDef[1].mcName);
  metricCalcDef[1].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
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
  metricCalcDef[2].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
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
  metricCalcDef[3].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[3].mcChangeType=MD_GAUGE;
  metricCalcDef[3].mcIsContinuous=MD_TRUE;
  metricCalcDef[3].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[3].mcDataType=MD_FLOAT32;
  metricCalcDef[3].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[3].mcCalc=metricCalcErrorRate;
  metricCalcDef[3].mcUnits=muErrorsPerSecond;

  metricCalcDef[4].mcVersion=MD_VERSION;
  metricCalcDef[4].mcName="PacketsTransmitted";
  metricCalcDef[4].mcId=mr(pluginname,metricCalcDef[4].mcName);
  metricCalcDef[4].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[4].mcChangeType=MD_GAUGE;
  metricCalcDef[4].mcIsContinuous=MD_TRUE;
  metricCalcDef[4].mcCalculable=MD_SUMMABLE;
  metricCalcDef[4].mcDataType=MD_UINT64;
  metricCalcDef[4].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[4].mcCalc=metricCalcPacketsTransmitted;
  metricCalcDef[4].mcUnits=muPackets;

  metricCalcDef[5].mcVersion=MD_VERSION;
  metricCalcDef[5].mcName="PacketsReceived";
  metricCalcDef[5].mcId=mr(pluginname,metricCalcDef[5].mcName);
  metricCalcDef[5].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[5].mcChangeType=MD_GAUGE;
  metricCalcDef[5].mcIsContinuous=MD_TRUE;
  metricCalcDef[5].mcCalculable=MD_SUMMABLE;
  metricCalcDef[5].mcDataType=MD_UINT64;
  metricCalcDef[5].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[5].mcCalc=metricCalcPacketsReceived;
  metricCalcDef[5].mcUnits=muPackets;

  metricCalcDef[6].mcVersion=MD_VERSION;
  metricCalcDef[6].mcName="PacketTransmitRate";
  metricCalcDef[6].mcId=mr(pluginname,metricCalcDef[6].mcName);
  metricCalcDef[6].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[6].mcChangeType=MD_GAUGE;
  metricCalcDef[6].mcIsContinuous=MD_TRUE;
  metricCalcDef[6].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[6].mcDataType=MD_UINT32;
  metricCalcDef[6].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[6].mcCalc=metricCalcPacketTransRate;
  metricCalcDef[6].mcUnits=muPacketsPerSecond;

  metricCalcDef[7].mcVersion=MD_VERSION;
  metricCalcDef[7].mcName="PacketReceiveRate";
  metricCalcDef[7].mcId=mr(pluginname,metricCalcDef[7].mcName);
  metricCalcDef[7].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[7].mcChangeType=MD_GAUGE;
  metricCalcDef[7].mcIsContinuous=MD_TRUE;
  metricCalcDef[7].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[7].mcDataType=MD_UINT32;
  metricCalcDef[7].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[7].mcCalc=metricCalcPacketRecRate;
  metricCalcDef[7].mcUnits=muPacketsPerSecond;

  metricCalcDef[8].mcVersion=MD_VERSION;
  metricCalcDef[8].mcName="ByteTransmitRate";
  metricCalcDef[8].mcId=mr(pluginname,metricCalcDef[8].mcName);
  metricCalcDef[8].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[8].mcChangeType=MD_GAUGE;
  metricCalcDef[8].mcIsContinuous=MD_TRUE;
  metricCalcDef[8].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[8].mcDataType=MD_UINT32;
  metricCalcDef[8].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[8].mcCalc=metricCalcByteTransRate;
  metricCalcDef[8].mcUnits=muBytesPerSecond;

  metricCalcDef[9].mcVersion=MD_VERSION;
  metricCalcDef[9].mcName="ByteReceiveRate";
  metricCalcDef[9].mcId=mr(pluginname,metricCalcDef[9].mcName);
  metricCalcDef[9].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[9].mcChangeType=MD_GAUGE;
  metricCalcDef[9].mcIsContinuous=MD_TRUE;
  metricCalcDef[9].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[9].mcDataType=MD_UINT32;
  metricCalcDef[9].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[9].mcCalc=metricCalcByteRecRate;
  metricCalcDef[9].mcUnits=muBytesPerSecond;
  
  metricCalcDef[10].mcVersion=MD_VERSION;
  metricCalcDef[10].mcName="ReceivePacketsDropped";
  metricCalcDef[10].mcId=mr(pluginname,metricCalcDef[10].mcName);
  metricCalcDef[10].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[10].mcChangeType=MD_GAUGE;
  metricCalcDef[10].mcIsContinuous=MD_TRUE;
  metricCalcDef[10].mcCalculable=MD_SUMMABLE;
  metricCalcDef[10].mcDataType=MD_UINT64;
  metricCalcDef[10].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[10].mcCalc=metricCalcReceiveDrop;
  metricCalcDef[10].mcUnits=muPackets;

  metricCalcDef[11].mcVersion=MD_VERSION;
  metricCalcDef[11].mcName="TransmitPacketsDropped";
  metricCalcDef[11].mcId=mr(pluginname,metricCalcDef[11].mcName);
  metricCalcDef[11].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_INTERVAL|MD_ORGSBLIM;
  metricCalcDef[11].mcChangeType=MD_GAUGE;
  metricCalcDef[11].mcIsContinuous=MD_TRUE;
  metricCalcDef[11].mcCalculable=MD_SUMMABLE;
  metricCalcDef[11].mcDataType=MD_UINT64;
  metricCalcDef[11].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[11].mcCalc=metricCalcTransmitDrop;
  metricCalcDef[11].mcUnits=muPackets;

  metricCalcDef[12].mcVersion=MD_VERSION;
  metricCalcDef[12].mcName="ReceiveDropRate";
  metricCalcDef[12].mcId=mr(pluginname,metricCalcDef[12].mcName);
  metricCalcDef[12].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[12].mcChangeType=MD_GAUGE;
  metricCalcDef[12].mcIsContinuous=MD_TRUE;
  metricCalcDef[12].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[12].mcDataType=MD_UINT32;
  metricCalcDef[12].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[12].mcCalc=metricCalcRecDropRate;
  metricCalcDef[12].mcUnits=muPacketsPerSecond;

  metricCalcDef[13].mcVersion=MD_VERSION;
  metricCalcDef[13].mcName="TransmitDropRate";
  metricCalcDef[13].mcId=mr(pluginname,metricCalcDef[13].mcName);
  metricCalcDef[13].mcMetricType=MD_PERIODIC|MD_CALCULATED|MD_RATE|MD_ORGSBLIM;
  metricCalcDef[13].mcChangeType=MD_GAUGE;
  metricCalcDef[13].mcIsContinuous=MD_TRUE;
  metricCalcDef[13].mcCalculable=MD_NONSUMMABLE;
  metricCalcDef[13].mcDataType=MD_UINT32;
  metricCalcDef[13].mcAliasId=metricCalcDef[0].mcId;
  metricCalcDef[13].mcCalc=metricCalcTransDropRate;
  metricCalcDef[13].mcUnits=muPacketsPerSecond;

  *mcnum=14;
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

    b1 = ip_getData(mv[0].mvData, TX_BYTE);
    if( mnum > 1 ) {
      b2 = ip_getData(mv[mnum-1].mvData, TX_BYTE);
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

    b1 = ip_getData(mv[0].mvData, RX_BYTE);
    if( mnum > 1 ) {
      b2 = ip_getData(mv[mnum-1].mvData, RX_BYTE);
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
/* ByteTransmitRate                                                           */
/* ---------------------------------------------------------------------------*/

size_t metricCalcByteTransRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long b1, b2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketTransmitRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		b1 = ip_getData(mv[0].mvData, TX_BYTE);
		b2 = ip_getData(mv[mnum-1].mvData, TX_BYTE);
		
		rate = (b1 - b2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long ));
		return sizeof(unsigned long);
	}
	return -1;
}


/* ---------------------------------------------------------------------------*/
/* ByteReceiveRate                                                            */
/* ---------------------------------------------------------------------------*/

size_t metricCalcByteRecRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long b1, b2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketReceiveRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		b1 = ip_getData(mv[0].mvData, RX_BYTE);
		b2 = ip_getData(mv[mnum-1].mvData, RX_BYTE);
		
		rate = (b1 - b2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long));
		return sizeof(unsigned long);
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

    et1 = ip_getData(mv[0].mvData, TX_ERRS);
    er1 = ip_getData(mv[0].mvData, RX_ERRS);

    et2 = ip_getData(mv[mnum-1].mvData, TX_ERRS);
    er2 = ip_getData(mv[mnum-1].mvData, RX_ERRS);

    rate = ( (et1+er1) - (et2+er2) ) /
           (mv[0].mvTimeStamp - mv[mnum-1].mvTimeStamp);

    //fprintf(stderr,"error rate: %f\n",rate);
    memcpy(v,&rate,sizeof(float));
    return sizeof(float);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PacketTransmitRate                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPacketTransRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long p1, p2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketTransmitRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		p1 = ip_getData(mv[0].mvData, TX_PACK);
		p2 = ip_getData(mv[mnum-1].mvData, TX_PACK);
		
		rate = (p1 - p2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long ));
		return sizeof(unsigned long);
	}
	return -1;
}


/* ---------------------------------------------------------------------------*/
/* PacketReceiveRate                                                          */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPacketRecRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long p1, p2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketReceiveRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		p1 = ip_getData(mv[0].mvData, RX_PACK);
		p2 = ip_getData(mv[mnum-1].mvData, RX_PACK);
		
		rate = (p1 - p2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long));
		return sizeof(unsigned long);
	}
	return -1;
}


/* ---------------------------------------------------------------------------*/
/* PacketsTransmitted                                                         */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPacketsTransmitted( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) {
  unsigned long long pt = 0;
  unsigned long long p1 = 0;
  unsigned long long p2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketsTransmitted\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    p1 = ip_getData(mv[0].mvData, TX_PACK);
    if( mnum > 1 ) {
      p2 = ip_getData(mv[mnum-1].mvData, TX_PACK);
      pt = p1-p2;
    }
    else { pt = p1; }

    //fprintf(stderr,"packets transmitted: %lld\n",pt);
    memcpy(v,&pt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* PacketsReceived                                                            */
/* ---------------------------------------------------------------------------*/

size_t metricCalcPacketsReceived( MetricValue *mv,   
				  int mnum,
				  void *v, 
				  size_t vlen ) {
  unsigned long long pr = 0;
  unsigned long long p1 = 0;
  unsigned long long p2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketsReceived\n",
	  __FILE__,__LINE__);
#endif

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    p1 = ip_getData(mv[0].mvData, RX_PACK);
    if( mnum > 1 ) {
      p2 = ip_getData(mv[mnum-1].mvData, RX_PACK);
      pr = p1-p2;
    }
    else { pr = p1; }

    //fprintf(stderr,"packets received: %lld\n",pr);
    memcpy(v,&pr,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* ReceivePacketsDropped                                                      */
/* ---------------------------------------------------------------------------*/

size_t metricCalcReceiveDrop( MetricValue *mv,   
				  int mnum,
				  void *v, 
				  size_t vlen ) {
  unsigned long long pr = 0;
  unsigned long long p1 = 0;
  unsigned long long p2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketsReceived\n",
	  __FILE__,__LINE__);
#endif

  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    p1 = ip_getData(mv[0].mvData, RX_DROP);
    if( mnum > 1 ) {
      p2 = ip_getData(mv[mnum-1].mvData, RX_DROP);
      pr = p1-p2;
    }
    else { pr = p1; }

    //fprintf(stderr,"packets received: %lld\n",pr);
    memcpy(v,&pr,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* TransmitPacketsDropped                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTransmitDrop( MetricValue *mv,   
				     int mnum,
				     void *v, 
				     size_t vlen ) {
  unsigned long long pt = 0;
  unsigned long long p1 = 0;
  unsigned long long p2 = 0;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketsTransmitted\n",
	  __FILE__,__LINE__);
#endif  
  if ( mv && (vlen>=sizeof(unsigned long long)) && (mnum>=1) ) {

    p1 = ip_getData(mv[0].mvData, TX_DROP);
    if( mnum > 1 ) {
      p2 = ip_getData(mv[mnum-1].mvData, TX_DROP);
      pt = p1-p2;
    }
    else { pt = p1; }

    //fprintf(stderr,"packets transmitted: %lld\n",pt);
    memcpy(v,&pt,sizeof(unsigned long long));
    return sizeof(unsigned long long);
  }
  return -1;
}


/* ---------------------------------------------------------------------------*/
/* TransmitPacketDropRate                                                     */
/* ---------------------------------------------------------------------------*/

size_t metricCalcTransDropRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long p1, p2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketTransmitRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		p1 = ip_getData(mv[0].mvData, TX_DROP);
		p2 = ip_getData(mv[mnum-1].mvData, TX_DROP);
		
		rate = (p1 - p2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long ));
		return sizeof(unsigned long);
	}
	return -1;
}


/* ---------------------------------------------------------------------------*/
/* ReceivePacketDropRate                                                      */
/* ---------------------------------------------------------------------------*/

size_t metricCalcRecDropRate(MetricValue * mv,
								 int mnum,
								 void * v,
								 size_t vlen)
{
	unsigned long long p1, p2;
	unsigned long rate;

#ifdef DEBUG
  fprintf(stderr,"--- %s(%i) : Calculate PacketTransmitRate\n",
	  __FILE__,__LINE__);
#endif
	if (mv && (vlen >= sizeof(unsigned long)) && (mnum >= 2)) {
		p1 = ip_getData(mv[0].mvData, RX_DROP);
		p2 = ip_getData(mv[mnum-1].mvData, RX_DROP);
		
		rate = (p1 - p2) / (mv[0].mvTimeStamp - mv[mnum - 1].mvTimeStamp);
		
		memcpy(v, &rate, sizeof(unsigned long ));
		return sizeof(unsigned long);
	}
	return -1;
}


/* ---------------------------------------------------------------------------*/
/* tool functions on BytesSubmitted                                           */
/* ---------------------------------------------------------------------------*/

unsigned long long ip_getData(char * data, char index) {
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
/*                        end of repositoryNetworkPort.c                      */
/* ---------------------------------------------------------------------------*/
