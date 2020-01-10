/*
 * $Id: metric.h,v 1.12 2009/05/20 19:39:55 tyreld Exp $
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
 * Description: Metric Defintiona and Value data types.
 *
 */

#ifndef METRIC_H
#define METRIC_H

#ifdef HAVE_CONFIG_H
#include "gather-config.h"
#ifndef SIZEOF_LONG
#error "SIZEOF_LONG must be defined for compilation."
#endif

#endif
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MD_RETRIEVED       0x0001
#define MD_CALCULATED      0x0002

// CIM_BaseMetricDefinition.GatheringType
#define MD_PERIODIC        0x0010

// CIM_BaseMetricDefinition.TimeScope
#define MD_POINT           0x0100   // 2 (Point)
#define MD_INTERVAL        0x0200   // 3 (Interval)
#define MD_STARTUPINTERVAL 0x0400   // 4 (StartupInterval)

#define MD_RATE            0x0800   // 32768 (Rate)
#define MD_AVERAGE         0x1000   // 32769 (Average)


// CIM_BaseMetricDefinition.IsContinuous
#define MD_FALSE           0x0000
#define MD_TRUE            0x0001


// CIM_BaseMetricDefinition.ChangeType
#define MD_COUNTER         0x0001   // 3 (Counter)
#define MD_GAUGE           0x0002   // 4 (Gauge)


// CIM_BaseMetricDefinition.DataType
#define MD_BOOL      0x0000
#define MD_UINT      0x0100
#define MD_SINT      0x0200
#define MD_FLOAT     0x0400
#define MD_CHAR16    0x0800
#define MD_DATETIME  0x1000
#define MD_STRING    0x2000
#define MD_USER      0x4000

#define MD_8BIT      0x0000
#define MD_16BIT     0x0001
#define MD_32BIT     0x0002
#define MD_64BIT     0x0004

#define MD_UINT8     (MD_UINT|MD_8BIT)
#define MD_UINT16    (MD_UINT|MD_16BIT)
#define MD_UINT32    (MD_UINT|MD_32BIT)
#define MD_UINT64    (MD_UINT|MD_64BIT)
#define MD_SINT8     (MD_SINT|MD_8BIT)
#define MD_SINT16    (MD_SINT|MD_16BIT)
#define MD_SINT32    (MD_SINT|MD_32BIT)
#define MD_SINT64    (MD_SINT|MD_64BIT)
#define MD_FLOAT32   (MD_FLOAT|MD_32BIT)
#define MD_FLOAT64   (MD_FLOAT|MD_64BIT)

#define MD_VERSION_MAJOR     0x0100
#define MD_VERSION_MINOR     0x0099
#define MD_VERSION_MINOR_MAX 0x00ff
#define MD_VERSION (MD_VERSION_MAJOR+MD_VERSION_MINOR)

/* CIM_BaseMetricDefinition.Calculable */
#define MD_NONCALCULABLE  1
#define MD_SUMMABLE       2
#define MD_NONSUMMABLE    3

typedef struct _MetricValue {
  int       mvId;       /* Metric Id */
#if SIZEOF_LONG == 8
  int       mv64Pad1;   /* explicit padding for 64 bit arch */
#endif
  time_t    mvTimeStamp;
  char     *mvResource; /* Resource (measured element)  name */
  unsigned  mvDataType;
#if SIZEOF_LONG == 8
  int       mv64Pad2;   /* explicit padding */
#endif
  size_t    mvDataLength;
  char     *mvData;
  char     *mvSystemId; /* System Id - this is needed for remote support */
} MetricValue;

/* support decls for mixed architectures */

typedef struct _MetricValue64 {
  int                mv64Id;      
  int                mv64Filler1;   /* padding */
  unsigned long long mv64TimeStamp; /* time_t 64 bit */
  unsigned long long mv64Resource;  /* char*  64 bit */
  unsigned           mv64DataType;  
  int                mv64Filler2;   /* padding */
  unsigned long long mv64DataLength;/* size_t 64 bit */
  unsigned long long mv64Data;      /* char*  64 bit */
  unsigned long long mv64SystemId;  /* char*  64 bit */
} MetricValue64;

typedef struct _MetricValue32 {
  int       mv32Id;      
  unsigned  mv32TimeStamp;  /* time_t 32 bit */
  unsigned  mv32Resource;   /* char*  32 bit */
  unsigned  mv32DataType;  
  unsigned  mv32DataLength; /* size_t 32 bit */
  unsigned  mv32Data;       /* char*  32 bit */
  unsigned  mv32SystemId;   /* char*  32 bit */
} MetricValue32;

/* data returning callback for plugin */
typedef int (MetricReturner) (MetricValue *mv); 

typedef int (MetricRetriever) (int mid, MetricReturner mret);
typedef void (MetricDeallocator) (void *v);
typedef int (ResourceLister) (int mid, char *** list);
typedef void (ResourceListDeallocator) (char ** list);
typedef size_t (MetricCalculator) (MetricValue *mv, int mnum,
				   void *v, size_t vlen);
/* Gatherer V1 Metric Definition
 */

typedef struct _MetricDefinitionV1 {
  char              *mdName;           /* Metric Descriptive Name */
  int                mdId;             /* Metric Id */
  time_t             mdSampleInterval;
  int                mdMetricType;
  int                mdAliasId;        /* Alias for computed metrics */
  unsigned           mdDataType;
  MetricRetriever   *mproc;
  MetricDeallocator *mdeal;
  MetricCalculator  *mcalc;
  ResourceLister    *mresl;
  ResourceListDeallocator *mresldeal;
} MetricDefinitionV1;

typedef struct _MetricDefinition {
  short              mdVersion;          /* New: mandatory version */
  char              *mdName;           /* Metric Descriptive Name */
  char              *mdReposPluginName;/* Name of repository plugin (lib) */
  int                mdId;             /* Metric Id */
  time_t             mdSampleInterval;
  MetricRetriever   *mproc;
  MetricDeallocator *mdeal;
} MetricDefinition;

typedef struct _MetricCalculationDefinition {
  short             mcVersion;
  char             *mcName;
  int               mcId;
  int               mcAliasId;
  int               mcMetricType;
  int               mcChangeType;
  unsigned char     mcIsContinuous;
  int               mcCalculable;
  int               mcDataType;
  char             *mcCimPlugin;  /* CIM Name Translation Plugin */
  char             *mcUnits;      
  MetricCalculator *mcCalc;
} MetricCalculationDefinition;

#ifdef __cplusplus
}
#endif

#endif
