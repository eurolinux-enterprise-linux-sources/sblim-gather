/*
 * $Id: mcfg.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Simple Configuration Support
 *
 */

#include "mcfg.h"
#include "mlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CFG_MAXHANDLE 10

static struct CFG_ITEM {
  char * ci_key;
  char * ci_value;
} * CfgRoot[CFG_MAXHANDLE] = {NULL};

static int CfgFiles=0;

static int _in_allowed(const char ** allowed,const char *key)
{
  while (allowed && *allowed) {
    if (strcasecmp(*allowed++,key)==0) {
      return 1;
    }
  }
  return 0;
}

int set_configfile(const char * filename, const char ** keys)
{
  FILE * cfgf;
  char   buffer[1000];
  int    skipblanks;
  char * separator;
  char * value;
  int    i=0;

  if (filename && keys) {
    cfgf = fopen(filename,"r");
    if (cfgf) {
      CfgFiles++;
      if (CfgFiles >= CFG_MAXHANDLE) {
	m_log(M_ERROR,M_QUIET,
	      "set_configfile: maximum number (%d) of config files exceeded",
	      CFG_MAXHANDLE);
	return -1;
      }
      while (!feof(cfgf)) {
	if (fgets(buffer,sizeof(buffer),cfgf)) {
	  skipblanks=strspn(buffer," \t");
	  if (buffer[skipblanks]=='#') {
	    /* comment */
	    continue;
	  } else {
	    separator = strchr(buffer+skipblanks,'=');
	    if (separator) {
	      *separator=' ';
	      value=separator+1;
	      if (value[strlen(value)-1]=='\n') {
		value[strlen(value)-1]=0;
	      }
	      while (separator > buffer+skipblanks) {
		if (isspace(*separator)) {
		  *separator-- = 0;
		} else {
		  break;
		}
	      }
	      if (_in_allowed(keys,buffer+skipblanks)) {
		/* extend config buffer */
		CfgRoot[CfgFiles] = 
		  realloc(CfgRoot[CfgFiles],sizeof(struct CFG_ITEM)*(i+2));
		CfgRoot[CfgFiles][i].ci_key = strdup(buffer+skipblanks);
		CfgRoot[CfgFiles][i].ci_value = strdup(value);
		CfgRoot[CfgFiles][i+1].ci_key=NULL;
		i++;
	      }
	    }
	  }
	}
      }
      fclose(cfgf);
      return CfgFiles;
    }
  }
  return -1;
}

int get_configitem(int handle, const char * key, char *value, size_t maxlen)
{
  int i=0;
  if (handle > 0 && handle < CFG_MAXHANDLE && CfgRoot[handle] && key && value) {
    while (CfgRoot[handle][i].ci_key) {
      if (strcasecmp(CfgRoot[handle][i].ci_key,key)==0 && 
	  strlen(CfgRoot[handle][i].ci_value) < maxlen) {
	strcpy(value,CfgRoot[handle][i].ci_value);
	return 0;
      }
      i++;
    }
  }
  return -1;
}
