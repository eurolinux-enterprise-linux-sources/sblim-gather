/*
 * $Id: reposdump.c,v 1.3 2009/05/20 19:39:55 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2007, 2009
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
 * Description:  Repository Dumper
 *  
 */

#define REPOSDUMP_USAGE     1
#define REPOSDUMP_NOTFOUND  2
#define REPOSDUMP_FILEWRITE 3

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "util/dirutil.h"
#include "util/commheap.h"
#include "util/reposcfg.h"
#include "repos.h"
#include "rrepos.h"


static void usage(const char * program);

/* type, value, attribute conversions */
static char* datatype_chars_unsafe(int datatype);
static char* metrictype_chars_unsafe(int metrictype);
static char* gatheringtype_chars_unsafe(int metrictype);
static char* timescope_chars_unsafe(int metrictype);
static char* changetype_chars_unsafe(int changetype);
static char* continuous_chars_unsafe(int continuous);
static char* calculable_chars_unsafe(int calculable);
static char* time_chars_unsafe(time_t t);
static char* value_chars_unsafe(int datatype, char * value);

int main(int argc, char * argv[])
{
  /* argument and setting handling */
  char plugindir[300];
  char expiration_str[300];
  int expiration = 0;
  int dumpage;
  /* output file handling */
  char fname[400];
  char *dumpdir;
  FILE * fhdl;
  /* rrepos API related */
  char **plugins;
  int  pnum;
  RepositoryPluginDefinition *pdef;
  COMMHEAP ch, ch2;
  ValueRequest vr;
  int iterations;
  int startdate;
  int enddate;
  int dumpinterval;
  /* current time */
  time_t now;
  char timestr[40];
  /* loop variables */
  int  i, j, k, l;


  /* check invocation arguments */
  if (argc > 4) {
    usage(argv[0]);
    return REPOSDUMP_USAGE;
  }
  
  /* determine configuration settings: plugin directory and repository expiration interval */
  if (reposcfg_init() == 0) {
    if (reposcfg_getitem("PluginDirectory",plugindir,sizeof(plugindir)-1) != 0) {
      strcpy(plugindir,REPOS_PLUGINDIR);
    }
    if (reposcfg_getitem("ExpirationInterval",expiration_str,sizeof(expiration_str)-1) != 0) {
      strcpy(expiration_str,"1200");
      expiration = atoi(expiration_str) * 2;
    }
  } else {
    fprintf(stderr, "Could not read configuration file (reposd.conf)\n");
    return REPOSDUMP_NOTFOUND;
  }
  
  /* set dump sample interval for non-point !!! metrics */
  if (argc > 2) {
    dumpinterval = atoi(argv[2]);
    if (dumpinterval < 60) {
      dumpinterval = 60;
      printf("Adjusted sample interval to %d.\n",dumpinterval);
    }
  } else {
    dumpinterval = expiration / 4;
  }
  
  /* set dump age (total interval) */
  if (argc > 1 ) {
    dumpage = atoi(argv[1]);
    if (dumpage < dumpinterval) {
      dumpage = dumpinterval;
      printf("Adjusted sample age to %d.\n",dumpage);
    }
  } else {
    dumpage = expiration;
  }

  printf("Dumping %d seconds in %d second increments.\n",dumpage,dumpinterval);

  /* we will need the current time later on */
  now = time(NULL);
  strcpy(timestr,time_chars_unsafe(now));

  /* construct output filename and open for writing */
  if (argc > 3) {
    dumpdir = "/tmp";
  } else {
    dumpdir = argv[3];
  }
  sprintf(fname,"%s/reposd-dump-%s.out",dumpdir,timestr);
  fhdl = fopen(fname,"w");
  if (fhdl == NULL) {
    fprintf(stderr, "Could not open %s for writing\n", fname);
    releasefilelist(plugins);
    return REPOSDUMP_FILEWRITE;
  }

  /* 
   * list all plugins, get the metric definitions and finally the values 
   */
  /* enumerate plugins */
  if (getfilelist(plugindir,"*.so",&plugins) == 0) {
    for (i=0; plugins[i] != NULL; i++) {
      /*
       * Dump metric definitions
       */
      /* initialize memory management and enumerate metric definitions */
      ch = ch_init();
      pnum = rreposplugin_list(plugins[i], &pdef, ch);
      if (pnum > 0) {
	fprintf(fhdl,"===metricdefs %s===\n", plugins[i]);
	fprintf(fhdl,"Name,Id,Data Type,Units,Metric Type,Gathering Type,Time Scope,Change Type,Is Continous,Calculable\n");
	for (j=0; j<pnum; j++) {
	  fprintf(fhdl,"%s,%d,%s,%s,%s,%s,%s,%s,%s,%s\n",
		  pdef[j].rdName,
		  pdef[j].rdId,
		  datatype_chars_unsafe(pdef[j].rdDataType),
		  pdef[j].rdUnits,
		  metrictype_chars_unsafe(pdef[j].rdMetricType),
		  gatheringtype_chars_unsafe(pdef[j].rdMetricType),
		  timescope_chars_unsafe(pdef[j].rdMetricType),
		  changetype_chars_unsafe(pdef[j].rdChangeType),
		  continuous_chars_unsafe(pdef[j].rdIsContinuous),
		  calculable_chars_unsafe(pdef[j].rdCalculable)
		  );
	}
      } else {
	fprintf(stderr,"No metric definitions found for %s. Is reposd running?\n",plugins[i]);
      }
      
      /* 
       * Dump metric values
       */
      if (pnum > 0) {
	fprintf(fhdl,"===metricvals %s===\n", plugins[i]);
	fprintf(fhdl,"Name,Id,System Id,Resource Id,Start Time,Duration,Value,Units\n");
	/* loop over metric definitions */
	for (j=0; j<pnum; j++) {
	  /* determine number of iterations based on age and interval*/
	  iterations = dumpage / dumpinterval + 1;
	  startdate = (now - dumpinterval) / dumpinterval * dumpinterval;
	  
	  for (l=0; l<iterations; l++) {
	    /* enumerate metrics for range: startdate - enddate */
	    enddate = startdate + dumpinterval;
	    vr.vsId = pdef[j].rdId;
	    vr.vsFrom = startdate;
	    vr.vsTo = enddate;
	    vr.vsSystemId = NULL;
	    vr.vsResource = NULL;
	    vr.vsNumValues = 1;
	    ch2 = ch_init();
	    if (rrepos_get(&vr,ch2) == 0) {
	      for (k=0; k<vr.vsNumValues; k++) {
		fprintf(fhdl,"%s,%d,%s,%s,%s,%ld,%s,%s\n",
			pdef[j].rdName,
			vr.vsId,
			vr.vsValues[k].viSystemId,
			vr.vsValues[k].viResource,
			time_chars_unsafe(vr.vsValues[k].viCaptureTime),
			vr.vsValues[k].viDuration,
			value_chars_unsafe(vr.vsDataType,vr.vsValues[k].viValue),
			pdef[j].rdUnits
			);
	      } 
	      ch_release(ch2);
	    } else {
	      ch_release(ch2);
	      break;
	    }
	    startdate = startdate - dumpinterval;
	  }
	}
      }
      ch_release(ch);
    }
    releasefilelist(plugins);
  } else {
    fprintf(stderr,"No plugins found in %s\n",plugindir);
    fclose(fhdl);
    return REPOSDUMP_NOTFOUND;
  }
  fclose(fhdl);
  return 0;
}

static void usage(const char * program)
{
  fprintf(stderr, "usage: %s [max-sample-age-in-seconds [sample-interval-in-seconds [dump-directory]]]\n", program);
}

static char* datatype_chars_unsafe(int datatype)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  switch(datatype) {
  case MD_BOOL:
    strcpy(buf,"Boolean");
    break;
  case MD_CHAR16:
    strcpy(buf,"Char16");
    break;
  case MD_STRING:
    strcpy(buf,"String");
    break;
  case MD_DATETIME:
    strcpy(buf,"Datetime");
    break;
  case MD_UINT8:
    strcpy(buf,"Uint8");
    break;
  case MD_UINT16:
    strcpy(buf,"Uint16");
    break;
  case MD_UINT32:
    strcpy(buf,"Uint32");
    break;
  case MD_UINT64:
    strcpy(buf,"Uint64");
    break;
  case MD_SINT8:
    strcpy(buf,"Sint8");
    break;
  case MD_SINT16:
    strcpy(buf,"Sint16");
    break;
  case MD_SINT32:
    strcpy(buf,"Sint32");
    break;
  case MD_SINT64:
    strcpy(buf,"Sint64");
    break;
  case MD_FLOAT32:
    strcpy(buf,"Float32");
    break;
  case MD_FLOAT64:
    strcpy(buf,"Float64");
    break;
  default:
    sprintf(buf,"*** unknown data type %x ***", datatype);
    break;
  };
  return buf;
}

static char* metrictype_chars_unsafe(int metrictype)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  if (metrictype & MD_RETRIEVED) {
    strcpy(buf,"Retrieved");
  } else if (metrictype & MD_CALCULATED) {
    strcpy(buf,"Calculated");
  } else {
    sprintf(buf,"*** unknown metric type %x ***", metrictype);
  };
  return buf;
}

static char* gatheringtype_chars_unsafe(int metrictype)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  if (metrictype & MD_PERIODIC) {
    strcpy(buf,"Periodic");
  } else {
    sprintf(buf,"*** unknown gathering type %x ***", metrictype);
  };
  return buf;
}

static char* timescope_chars_unsafe(int metrictype)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  if (metrictype & MD_POINT) {
    strcpy(buf,"Point");
  } else  if (metrictype & MD_INTERVAL) {
    strcpy(buf,"Interval");
  } else if (metrictype & MD_STARTUPINTERVAL) {
    strcpy(buf,"Startupinterval");
  } else  if (metrictype & MD_RATE) {
    strcpy(buf,"Rate");
  } else  if (metrictype & MD_AVERAGE) {
    strcpy(buf,"Average");
  } else {
    sprintf(buf,"*** unknown timescope %x ***", metrictype);
  };
  return buf;
}

static char* changetype_chars_unsafe(int changetype)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  switch(changetype) {
  case 0:
    strcpy(buf,"Unspecified");
    break;
  case MD_COUNTER:
    strcpy(buf,"Counter");
    break;
  case MD_GAUGE:
    strcpy(buf,"Gauge");
    break;
  default:
    sprintf(buf,"*** unknown change type %x ***", changetype);
    break;
  };
  return buf;
}

static char* continuous_chars_unsafe(int continuous)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  switch(continuous) {
  case MD_TRUE:
    strcpy(buf,"True");
    break;
  case MD_FALSE:
    strcpy(buf,"False");
    break;
  default:
    sprintf(buf,"*** unknown continuous state %x ***", continuous);
    break;
  };
  return buf;
}

static char* calculable_chars_unsafe(int calculable)
{
  /* not threadsafe - OK for this program */
  static char buf[100];
  switch(calculable) {
  case MD_NONCALCULABLE:
    strcpy(buf,"Noncalculable");
    break;
  case MD_SUMMABLE:
    strcpy(buf,"Summable");
    break;
  case MD_NONSUMMABLE:
    strcpy(buf,"Nonsummable");
    break;
  default:
    sprintf(buf,"*** unknown calculable state %x ***", calculable);
    break;
  };
  return buf;
}

static char * time_chars_unsafe(time_t t)
{
  static char buf[300];
  struct tm *tp = gmtime(&t);
  sprintf(buf,"%04d%02d%02d%02d%02d%02d",
	  tp->tm_year + 1900,
	  tp->tm_mon + 1,
	  tp->tm_mday,
	  tp->tm_hour,
	  tp->tm_min,
	  tp->tm_sec
	  );
  return buf;
}

static char* value_chars_unsafe(int datatype, char * value)
{
  static char buf[300];
  char * bptr = buf;
  
  switch(datatype) {
  case MD_BOOL:
    sprintf(buf,"%s", *value ? "true" : "false" );
    break;
  case MD_SINT8:
    sprintf(buf,"%hhd",*value);
    break;
  case MD_UINT8:
    sprintf(buf,"%hhu",*(unsigned char*)value);
    break;
  case MD_UINT16:
  case MD_CHAR16:
    sprintf(buf,"%hu",*(unsigned short*)value);
    break;
  case MD_SINT16:
    sprintf(buf,"%hd",*(short*)value);
    break;
  case MD_UINT32:
    sprintf(buf,"%u",*(unsigned*)value);
    break;
  case MD_SINT32:
    sprintf(buf,"%d",*(int*)value);
    break;
  case MD_UINT64:
    sprintf(buf,"%llu",*(unsigned long long*)value);
    break;
  case MD_SINT64:
    sprintf(buf,"%lld",*(long long*)value);
    break;
  case MD_FLOAT32:
    sprintf(buf,"%f",*(float*)value);
    break;
  case MD_FLOAT64:
    sprintf(buf,"%f",*(double*)value);
    break;
  case MD_STRING:
    bptr = value;
    break;
  default:
    sprintf(buf,"*** datatype %0x not supported ***",datatype);
    break;
  }
  return bptr;
}
