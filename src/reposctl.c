/*
 * $Id: reposctl.c,v 1.18 2009/05/20 19:39:55 tyreld Exp $
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
 * Description:  Repository Remote Controller
 * Command line interface to the repository daemon.
 *  
 */

#include "metric.h"
#include "rrepos.h"
#include "mtrace.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char* commands[] = {
  "\th\t\tprint this help message\n",
  "\ts\t\tstatus\n",
  "\ti\t\tinit\n",
  "\tt\t\tterminate\n",
  "\tl plugin\tload plugin\n",
  "\tu plugin\tunload plugin\n",
  "\tv plugin\tview/list metrics for plugin\n",
  "\tb\t\tsubscribe\n",
  "\tn\t\tunsubscribe\n",
  "\tq\t\tquit\n",
  "\tg id [system [resource [from [to]]]]\tget metric value\n",
  "\tf id num asc [system [resource [from [to]]]]\tget filtered (top-n, sorted) metric value\n",
  "\tr id\tlist resources for metric\n",
  "\tG\t\tget global filter\n",
  "\tF num asc\tset global filter\n",
  "\tk\t\tkill daemon\n",
  "\td\t\tstart daemon\n",
  "\tc\t\tlocal trace\n",
  NULL
};

static void printhelp();
static void printvalue(ValueRequest *vr);
static void test_callback(int corrid, ValueRequest *vr);


int main()
{
  char              cmd;
  char             *arg;
  char              buf[500];
  char              argbuf[500];
  char              argbuf2[500];
  int               quit=0;
  int               i;
  RepositoryStatus  rs;
  int               pnum;
  COMMHEAP          commh;
  int               offFrom, offTo;
  size_t            num;
  int               ascending;
  ValueRequest      vr;
  SubscriptionRequest sr;
  RepositoryPluginDefinition *rdef;
  MetricResourceId  *resid;
  
  while(fgets(buf,sizeof(buf),stdin)) {
    cmd = buf[0];
    arg = buf+1;
    switch(cmd) {
    case 'h':
      printhelp();
      break;
    case 'c':
#ifndef NOTRACE
      m_trace_setlevel(MTRACE_ALL);
      m_trace_enable(MTRACE_MASKALL);
#endif
      break;
    case 's':
      if (rrepos_status(&rs) == 0) {
	printf("Status %sinitialized," 
	       " %d plugins and %d metrics. \n",
	       rs.rsInitialized?"":"NOT ",
	       rs.rsNumPlugins, rs.rsNumMetrics);
      } else {
	printf("Daemon not reachable.\n");
      }
      break;
    case 'i':
      if(rrepos_init())
	printf("Failed\n");
      break;
    case 't':
      if(rrepos_terminate())
	printf("Failed\n");
      break;
    case 'l':
      sscanf(arg,"%s",argbuf);
      if (rreposplugin_add(argbuf))
	printf("Failed\n");
      break;
    case 'u':
      sscanf(arg,"%s",argbuf);
      if (rreposplugin_remove(argbuf))
	printf("Failed\n");
      break;
    case 'v':
      commh=ch_init();
      sscanf(arg,"%s",argbuf);
      pnum=rreposplugin_list(argbuf,&rdef,commh);
      if (pnum < 0) {
	printf("Failed\n");
      } else {
	for (i=0;i<pnum;i++) {
	  printf("Plugin metric \"%s\" has id %d, units %s and data type %x\n",
		 rdef[i].rdName, rdef[i].rdId, rdef[i].rdUnits, rdef[i].rdDataType);
	}
      }
      ch_release(commh);
      break;
    case 'g':
      vr.vsId = 0;
      offFrom = 0;
      offTo  = 0;
      vr.vsSystemId = argbuf;
      vr.vsSystemId[0]=0;
      vr.vsResource = argbuf2;
      vr.vsResource[0]=0;
      vr.vsNumValues = 0;
      sscanf(arg,"%d %s %s %d %d ",&vr.vsId,
	     vr.vsSystemId,vr.vsResource,&offFrom,&offTo );
      if (strlen(vr.vsSystemId)==0 || vr.vsSystemId[0]=='*') {
	vr.vsSystemId=NULL;
      }
      if (strlen(vr.vsResource)==0 || vr.vsResource[0]=='*') 
	vr.vsResource=NULL;
      if (offTo) 
	vr.vsTo = time(NULL) + offTo;
      else
	vr.vsTo = 0;
      if (offFrom) 
	vr.vsFrom = time(NULL) + offFrom;
      else
	vr.vsFrom = 0;
      commh=ch_init();
      if (rrepos_get(&vr,commh)) {
	printf("Failed\n");
      } else {
	printvalue(&vr);
      }
      ch_release(commh);
      break;
    case 'f':
      vr.vsId = 0;
      offFrom = 0;
      offTo  = 0;
      vr.vsSystemId = argbuf;
      vr.vsSystemId[0]=0;
      vr.vsResource = argbuf2;
      vr.vsResource[0]=0;
      vr.vsNumValues = 0;
      sscanf(arg,"%d %zd %d %s %s %d %d ",&vr.vsId,
	     &num, &ascending,
	     vr.vsSystemId,vr.vsResource,&offFrom,&offTo );
      if (strlen(vr.vsSystemId)==0 || vr.vsSystemId[0]=='*') {
	vr.vsSystemId=NULL;
      }
      if (strlen(vr.vsResource)==0 || vr.vsResource[0]=='*') 
	vr.vsResource=NULL;
      if (offTo) 
	vr.vsTo = time(NULL) + offTo;
      else
	vr.vsTo = 0;
      if (offFrom) 
	vr.vsFrom = time(NULL) + offFrom;
      else
	vr.vsFrom = 0;
      commh=ch_init();
      if (rrepos_getfiltered(&vr,commh,num,ascending)) {
	printf("Failed\n");
      } else {
	printvalue(&vr);
      }
      ch_release(commh);
      break;
    case 'r':
      commh=ch_init();
      sscanf(arg,"%s",argbuf);
      pnum = rreposresource_list(argbuf,&resid,commh);
      if (pnum >= 0) {
	printf("Resources for metric %s:\n",argbuf);
	for (i=0;i<pnum;i++) {
	  printf("System=\"%s\", Resource=\"%s\"\n",resid[i].mrid_system, resid[i].mrid_resource);
	}
      } else {
	printf("Failed\n");
      }
      ch_release(commh);
      break;
    case 'G':
      if(rrepos_getglobalfilter(&num,&ascending)==0) {
	printf("Limit=%zd, order=%s.\n",num,ascending?"ascending":"descending");
      } else {
	printf("Failed\n");
      }
      break;
    case 'F':
      num=ascending=0;
      sscanf(arg,"%zd %d",&num,&ascending);
      if(rrepos_setglobalfilter(num,ascending)) {
	printf("Failed\n");
      }
      break;
    case 'b':
      sscanf(arg,"%s",argbuf);
      sr.srMetricId = atoi(arg);
      sr.srCorrelatorId = 1;
      sr.srSystemId = NULL;
      sr.srSystemOp = SUBSCR_OP_ANY;
      sr.srResource = NULL;
      sr.srResourceOp = SUBSCR_OP_ANY;
      sr.srValue = NULL;
      sr.srValueOp = SUBSCR_OP_ANY;
      if(rrepos_subscribe(&sr,test_callback))
	printf("Failed\n");
      break;
    case 'n':
      sscanf(arg,"%s",argbuf);
      sr.srMetricId = atoi(arg);
      sr.srCorrelatorId = 1;
      sr.srSystemId = NULL;
      sr.srSystemOp = SUBSCR_OP_ANY;
      sr.srResource = NULL;
      sr.srResourceOp = SUBSCR_OP_ANY;
      sr.srValue = NULL;
      sr.srValueOp = SUBSCR_OP_ANY;
      if(rrepos_unsubscribe(&sr,test_callback))
	printf("Failed\n");
      break;
    case 'k':
      if(rrepos_unload())
	printf("Failed to unload daemon\n");
      break;
    case 'd':
      if(rrepos_load())
	printf("Failed to load daemon\n");
      break;
    case 'q':
      quit=1;
      break;
    default:
      printhelp();
      break;
    }
    if (quit) break;
  }
  
  return 0;
}

static void printhelp()
{
  int i;
  for (i=0;commands[i];i++)
    printf(commands[i]);
}

static void printvalue(ValueRequest *vr)
{
  int i;
  printf("Value id %d has value data \n",
	 vr->vsId);
  for (i=0; i < vr->vsNumValues; i++) {
    printf("\t on %s for resource %s ",vr->vsValues[i].viSystemId,vr->vsValues[i].viResource);
    switch(vr->vsDataType) {
    case MD_BOOL:
      printf("%s", *vr->vsValues[i].viValue ? "true" : "false" );
      break;
    case MD_SINT8:
      printf("%hhd",*vr->vsValues[i].viValue);
      break;
    case MD_UINT8:
      printf("%hhu",*(unsigned char*)vr->vsValues[i].viValue);
      break;
    case MD_UINT16:
    case MD_CHAR16:
      printf("%hu",*(unsigned short*)vr->vsValues[i].viValue);
      break;
    case MD_SINT16:
      printf("%hd",*(short*)vr->vsValues[i].viValue);
      break;
    case MD_UINT32:
      printf("%u",*(unsigned*)vr->vsValues[i].viValue);
      break;
    case MD_SINT32:
      printf("%d",*(int*)vr->vsValues[i].viValue);
      break;
    case MD_UINT64:
      printf("%llu",*(unsigned long long*)vr->vsValues[i].viValue);
      break;
    case MD_SINT64:
      printf("%lld",*(long long*)vr->vsValues[i].viValue);
      break;
    case MD_FLOAT32:
      printf("%f",*(float*)vr->vsValues[i].viValue);
      break;
    case MD_FLOAT64:
      printf("%f",*(double*)vr->vsValues[i].viValue);
      break;
    case MD_STRING:
      printf(vr->vsValues[i].viValue);
      break;
    default:
      printf("datatype %0x not supported",vr->vsDataType);
      break;
    }
    printf(", sample time %s duration %ld",
	   ctime(&vr->vsValues[i].viCaptureTime),
	   vr->vsValues[i].viDuration);
    printf ("\n");
  }
}

static void test_callback(int corrid, ValueRequest *vr)
{
  printf("*** event notification: ");
  printf("timestamp=%s ", ctime(&vr->vsValues[0].viCaptureTime));
  printf("correlation id=%d ", corrid);
  printf("metric id=%d ",vr->vsId);
  printf("resource=%s ",vr->vsValues[0].viResource);
  printvalue(vr);
  printf("\n");
}

