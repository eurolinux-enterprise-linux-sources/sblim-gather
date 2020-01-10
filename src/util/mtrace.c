/*
 * $Id: mtrace.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors: Heidi Neumann <heidineu@de.ibm.com>
 *               Michael Schuele <schuelem@de.ibm.com>
 *
 * Description:
 * Trace Support based on the SBLIM OSBASE Providers
 */

#undef NOTRACE

#include "mtrace.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

unsigned    mtrace_mask  = 0;
int         mtrace_level = 0;
const char *mtrace_file  = NULL;

static char *mtrace_components[] = {
  "<reserved>",
  MTRACE_UTIL_S,
  MTRACE_COMM_S,
  MTRACE_REPOS_S,
  MTRACE_GATHER_S,
  MTRACE_RGATHER_S,
  MTRACE_RREPOS_S,
  NULL
};


static pthread_key_t  vsbuf_key;
static pthread_once_t vsbuf_once = PTHREAD_ONCE_INIT;

static void vsbuf_term(void *vsbuf)
{
  if (vsbuf) {
    free(vsbuf);
  }
}

static void vsbuf_init()
{
  pthread_key_create(&vsbuf_key,vsbuf_term);
}

static char* get_vsbuf(size_t len)
{
  char * vsbuf;

  pthread_once(&vsbuf_once,vsbuf_init);
  vsbuf = (char*)pthread_getspecific(vsbuf_key);
  if (vsbuf == NULL) {
    vsbuf = malloc(len);
    pthread_setspecific(vsbuf_key,vsbuf); 
  }
  return vsbuf;
}

char * _m_format_trace(const char *fmt,...) {
   va_list ap;
   char *msg=get_vsbuf(1024);

   va_start(ap,fmt);
   vsnprintf(msg,1024,fmt,ap);
   va_end(ap);
   return msg;
}

static int _f_trace(char * buf, size_t len, int level, unsigned component,
		    const char * filename, int lineno, const char * msg) 
{
  struct tm        cttm;
  struct timeval   tv;
  struct timezone  tz;
  time_t           sec  = 0;
  char             tm[20];
  static pid_t     pid=0;
  
  if( gettimeofday( &tv, &tz) == 0 ) {
    sec = tv.tv_sec + (tz.tz_minuteswest*-1*60);
    memset(tm, 0, sizeof(tm));
    if( gmtime_r( &sec , &cttm) != NULL ) {
      strftime(tm,20,"%m/%d/%Y %H:%M:%S UTC",&cttm);
    }
  }

  if (pid==0) {
    pid=getpid();
  }
  
  return snprintf(buf,len,"%s %s[%u:%ld] %s(%i) : %s\n", 
		  tm, m_trace_component(component), pid, pthread_self(), 
		  filename, lineno, msg); 
  
}

void _m_trace(int level, unsigned component, const char * filename, 
	      int lineno, const char * msg) 
{
  char   fbuf[1024];    
  FILE * ferr = NULL;
  
  if (level > 0 && level <=mtrace_level) {
    if( (mtrace_file != NULL) ) {
      if( (ferr=fopen(mtrace_file,"a")) == NULL ) {
	fprintf(stderr,"Couldn't open trace file");
	return;
      }
    }
    else { ferr = stderr; } 
    
    _f_trace(fbuf,sizeof(fbuf),level,component,filename,lineno,msg);
    fwrite(fbuf,strlen(fbuf),1,ferr);

    if( (mtrace_file != NULL) ) {
      fclose(ferr);
    }
  }
}

void m_trace(int level, unsigned component, const char * filename, 
	     int lineno, const char * fmt, ...)
{
  FILE   * ferr = NULL;
  va_list  ap;
  char   * msg=get_vsbuf(1024);

  if (level > 0 && level <=mtrace_level) {
    if( (mtrace_file != NULL) ) {
      if( (ferr=fopen(mtrace_file,"a")) == NULL ) {
	fprintf(stderr,"Couldn't open trace file");
	return;
      }
    }
    else { ferr = stderr; } 
     
    va_start(ap,fmt);
    vsnprintf(msg,1024,fmt,ap);
    va_end(ap);

    if( (mtrace_file != NULL) ) {
      fclose(ferr);
    }
  }
}

void m_trace_setfile(const char * tracefile)
{
  mtrace_file = strdup(tracefile);
}

void m_trace_setlevel(int level)
{
  mtrace_level = level;
}

void m_trace_enable(unsigned component)
{
  mtrace_mask |= component;
}

void m_trace_disable(unsigned component)
{
  mtrace_mask ^= component;
}

unsigned m_trace_compid(const char * component_name)
{
  int i=1;
  while (mtrace_components[i]) {
    if (strcmp(mtrace_components[i],component_name)==0) {
      return 0x0001 << (i-1);
    }
    i++;
  }
  return 0;
}

const char * m_trace_component(unsigned component)
{
  static int maxcomp = sizeof(mtrace_components)/sizeof(char*) - 1;
  int i;
  
  for (i=1; i<maxcomp; i++) {
    if (component == (0x0001 << (i-1))) {
      return mtrace_components[i];
    }
  }
  return "unknown";
}

