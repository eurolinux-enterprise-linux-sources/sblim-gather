/*
 * $Id: hypfs.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2006, 2009
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
 * Description: Hypervisor FS Support Code
 *
 */

#include "hypfs.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <mntent.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef DEBUG
#define TRACE_ENTER(name,parms) printf(">>> %s(%d) %s  ", __FILE__, __LINE__, #name), printf parms, printf("\n")
#define TRACE_LEAVE(name,retv) printf("<<< %s(%d) %s  ", __FILE__, __LINE__, #name), printf retv, printf("\n")
#define TRACE_LEAVE_V(name) printf("<<< %s(%d) %s\n", __FILE__, __LINE__, #name)
#define TRACE(name,parms)  printf("--- %s(%d) %s  ", __FILE__, __LINE__, #name), printf parms, printf("\n")
#else
#define TRACE_ENTER(name,parms)
#define TRACE_LEAVE(name,retv)
#define TRACE_LEAVE_V(name)
#define TRACE(name,parms)
#endif

#define HV_FILE_TIMESTAMP  "update"
#define HV_FILE_MGMTTIME   "mgmtime"
#define HV_FILE_CPUTIME    "cputime"
#define HV_FILE_ONLINETIME "onlinetime"
#define HV_FILE_PLATFORM   "hyp/type"
#define HV_FILE_TYPE       "type"

#define HV_DIR_CPU         "cpus"
#define HV_DIR_SYSTEM      "systems"

#define HV_TS_MAXSKEW      1
#define HV_TS_MAXTRY       2

static int hv_request_update(const char *tspath);

static int hv_get_timestamp(const char *tspath, time_t *ts);
static int hv_get_ull(const char *ullpath, unsigned long long *ull);
static int hv_get_string(const char *strpath, char *str, size_t slen);

static int  hv_get_systeminfo(const char *hypfspath, HypSystem *hypsys);
static void hv_release_systeminfo(HypSystem *hypsys);

static int  hv_get_dirnames(const char *basedir, char ***dirnames);
static void hv_release_dirnames(char **dirnames);

HypSystem * get_hypervisor_info(const char *hypfspath)
{
  int         i;
  time_t      t_now;
  time_t      t_update1, t_update2;
  HypSystem * hypsys = NULL;
  char        fullpath[PATH_MAX+1];
  
  TRACE_ENTER(get_hypervisor_info,("path=%s",hypfspath));

  if (snprintf(fullpath,PATH_MAX,"%s/%s",hypfspath,HV_FILE_TIMESTAMP) <= PATH_MAX &&
      hv_request_update(fullpath) == 0 && hv_get_timestamp(fullpath,&t_update1) == 0) {
    t_now = time(NULL);
    if (abs(t_update1-t_now) <= HV_TS_MAXSKEW) {
      for (i=0; i<HV_TS_MAXTRY;i++) {
	hypsys=calloc(1,sizeof(HypSystem));
	if (hypsys == NULL) {
	  TRACE(get_hypervisor_info,("calloc failed"));
	  break;
	}
        if (hv_get_systeminfo(hypfspath,hypsys) == 0 &&
	    hv_get_timestamp(fullpath,&t_update2) == 0 &&
	    t_update1 == t_update2) {
	  snprintf(fullpath,PATH_MAX,"%s/%s",hypfspath,HV_FILE_PLATFORM);
	  if (hv_get_string(fullpath,hypsys->hs_id,HYPFS_MAXNAME)) {
	    strcpy(hypsys->hs_id,"unknown");
	  }
	  hypsys->hs_timestamp = t_update2;
	  break;
	}
	release_hypervisor_info(hypsys);
	hypsys=NULL;
      }
    }
  }
  
  TRACE_LEAVE(get_hypervisor_info,("id=%s",hypsys?hypsys->hs_id:"<null>"));
  return hypsys;
}

void release_hypervisor_info(HypSystem * hypsys)
{
  TRACE_ENTER(release_hypervisor_info,("id=%s",hypsys?hypsys->hs_id:"<null>"));

  if (hypsys) {
    hv_release_systeminfo(hypsys);
    free(hypsys);
  }

  TRACE_LEAVE_V(release_hypervisor_info);
}

const char * get_hypervisor_rootpath(void)
{
  static char   str[PATH_MAX+1];
  char          buff[PATH_MAX*2+1];
  FILE         *fmnt;
  struct mntent me;
  TRACE_ENTER(get_hypervisor_rootpath,("void"));

  fmnt = setmntent("/etc/mtab","r");
  strcpy(str,HYPFS_CANONICAL_MOUNT);
  if (fmnt) {
    while(getmntent_r(fmnt,&me,buff,sizeof(buff))) {
      if (strcmp(me.mnt_type,"s390_hypfs") == 0) {
	strcpy(str,me.mnt_dir);
	break;
      }
    }
    endmntent(fmnt);
  }

  TRACE_LEAVE(get_hypervisor_rootpath,("rootpath=%s",str));
  return str;
}

static int  hv_get_systeminfo(const char *hypfspath, HypSystem *hypsys)
{
  int     i,j;
  int     skip;
  DIR    *dd;
  char  **entries;
  char    fullpath[PATH_MAX+1];
  
  TRACE_ENTER(hv_get_systeminfo,("path=%s",hypfspath));

  hypsys->hs_vtype = strcmp(hypfspath,get_hypervisor_rootpath()) ? HYPFS_VIRTUAL : HYPFS_PHYSICAL;
  
  /* read cpu info */
  if (snprintf(fullpath,PATH_MAX,"%s/%s",hypfspath,HV_DIR_CPU) <= PATH_MAX) {
    hypsys->hs_numcpu = hv_get_dirnames(fullpath,&entries);
    if ( hypsys->hs_numcpu > 0) {
      hypsys->hs_cpu = calloc(hypsys->hs_numcpu,sizeof(HypProcessor));
      if (hypsys->hs_cpu == NULL) {
	  TRACE(hv_get_systeminfo,("calloc failed"));
	  TRACE_LEAVE(hv_get_systeminfo,("rc=-1"));
	  return -1;
      }
      for (i=j=0; j < hypsys->hs_numcpu; i++, j++) {
	skip = 0;
	/* check if directory */
	snprintf(fullpath,PATH_MAX,"%s/%s/%s",hypfspath,HV_DIR_CPU,entries[i]);
	dd = opendir(fullpath);
	if (dd==NULL) {
	  skip = 1;
	} else {
	  closedir(dd);
	}
	if (skip) {
	  if (--hypsys->hs_numcpu == 0) {
	    free(hypsys->hs_cpu);
	    hypsys->hs_cpu = NULL;
	    break;
	  };
	  j -= 1;
	  continue;
	}
	/* get values */
	hypsys->hs_cpu[j].hp_vtype = hypsys->hs_vtype;
	strncpy(hypsys->hs_cpu[j].hp_id,entries[i],HYPFS_MAXNAME);
	snprintf(fullpath,PATH_MAX,"%s/%s/%s/%s",hypfspath,HV_DIR_CPU,entries[i],
		 HV_FILE_TYPE);
	if (hv_get_string(fullpath,hypsys->hs_cpu[j].hp_info,HYPFS_MAXNAME)) {
	  strcpy(hypsys->hs_cpu[j].hp_info,"n/a");
	}
	snprintf(fullpath,PATH_MAX,"%s/%s/%s/%s",hypfspath,HV_DIR_CPU,entries[i],
		 HV_FILE_MGMTTIME);
	if (hv_get_ull(fullpath,&hypsys->hs_cpu[j].hp_mgmttime)) {
	  hypsys->hs_cpu[j].hp_mgmttime = -1LL;
	}
	snprintf(fullpath,PATH_MAX,"%s/%s/%s/%s",hypfspath,HV_DIR_CPU,entries[i],
		 HV_FILE_CPUTIME);
	if (hv_get_ull(fullpath,&hypsys->hs_cpu[j].hp_cputime)) {
	  hypsys->hs_cpu[j].hp_cputime = -1LL;
	}
	snprintf(fullpath,PATH_MAX,"%s/%s/%s/%s",hypfspath,HV_DIR_CPU,entries[i],
		 HV_FILE_ONLINETIME);
	if (hv_get_ull(fullpath,&hypsys->hs_cpu[j].hp_onlinetime)) {
	  hypsys->hs_cpu[j].hp_onlinetime = -1LL;
	}
      }
      hv_release_dirnames(entries);
    } else if (hypsys->hs_numcpu < 0) {
	  TRACE(hv_get_systeminfo,("hv_get_dirnames failed"));
	  TRACE_LEAVE(hv_get_systeminfo,("rc=-1"));
	  return -1;
    }
  }
 
  /* read systems */
  if (snprintf(fullpath,PATH_MAX,"%s/%s",hypfspath,HV_DIR_SYSTEM) <= PATH_MAX) {
    hypsys->hs_numsys = hv_get_dirnames(fullpath,&entries);
    if ( hypsys->hs_numsys > 0) {
      hypsys->hs_sys = calloc(hypsys->hs_numsys,sizeof(HypSystem));
      if (hypsys->hs_sys == NULL) {
	  TRACE(hv_get_systeminfo,("calloc failed"));
	  TRACE_LEAVE(hv_get_systeminfo,("rc=-1"));
	  return -1;
      }
      for (i=j=0; j < hypsys->hs_numsys; i++, j++) {
	skip = 0;
	/* check if directory */
	snprintf(fullpath,PATH_MAX,"%s/%s/%s",hypfspath,HV_DIR_SYSTEM,entries[i]);
	dd = opendir(fullpath);
	if (dd==NULL) {
	  skip = 1;
	} else {
	  closedir(dd);
	}
	if (skip) {
	  if (--hypsys->hs_numsys == 0) {
	    free(hypsys->hs_sys);
	    hypsys->hs_sys = NULL;
	    break;
	  };
	  j -= 1;
	  continue;
	}
	/* get values */
	hypsys->hs_sys[j].hs_vtype = hypsys->hs_vtype;
	strncpy(hypsys->hs_sys[j].hs_id,entries[i],HYPFS_MAXNAME);
	hypsys->hs_sys[j].hs_timestamp = 0;
	hv_get_systeminfo(fullpath,hypsys->hs_sys+i);
      }
      hv_release_dirnames(entries);
    } else if (hypsys->hs_numsys < 0) {
	  TRACE(hv_get_systeminfo,("hv_get_dirnames failed"));
	  TRACE_LEAVE(hv_get_systeminfo,("rc=-1"));
	  return -1;
    }
  }

  TRACE_LEAVE(hv_get_systeminfo,("rc=0"));
  return 0;
}

static void hv_release_systeminfo(HypSystem *hypsys)
{
  int i;
  
  TRACE_ENTER(hv_release_systeminfo,("id=%s",hypsys?hypsys->hs_id:"<null>"));
  
  /* release cpus */
  if (hypsys->hs_cpu) {
    free(hypsys->hs_cpu);
  };
  /* release systems */
  if (hypsys->hs_sys) {
    for (i=0; i < hypsys->hs_numsys;i++) {
      hv_release_systeminfo(hypsys->hs_sys+i);
    }
    free(hypsys->hs_sys);
  }
  
  TRACE_LEAVE_V(hv_release_systeminfo);
}

static int hv_request_update(const char *tspath)
{
  int fd;
  int rc = 0;

  TRACE_ENTER(hv_request_update,("name=%s",tspath));
  
  fd = open(tspath,O_WRONLY);
  if (fd > 0) {  
#ifdef CHEATING
    time_t now;
    FILE * f;
    f = fdopen(fd,"w");
    if (f == NULL) {
      TRACE(hv_request_update,("error writing update file %d(%s)",errno,strerror(errno)));
      rc = -1;
    } else {
      now = time(NULL);
      fprintf(f,"%ld\n",now);
    }
    fclose(f);
#else
    if (write(fd,"1",1) != 1) {
      TRACE(hv_request_update,("error writing update file %d(%s)",errno,strerror(errno)));
      rc = -1;
    }
    close(fd);
#endif
  } else {
    rc = -1;
  }
  TRACE_LEAVE(hv_request_update,("rc=%d",rc));
  return rc;
}

static int hv_get_timestamp(const char *tspath, time_t *ts)
{
  int    rc;
  FILE * f;
  static int statmode=0;
  struct stat statbuf;
  
  TRACE_ENTER(hv_get_timestamp,("name=%s",tspath));
  rc = -1;
  if (statmode) {
    rc = stat(tspath,&statbuf);
    if (rc == 0) {
      *ts = statbuf.st_mtime;
    } else {
      TRACE(hv_request_update,("error stat'ing timestamp file %d(%s)",errno,strerror(errno)));    
    }
  } else {
    f = fopen(tspath,"r");
    if (f) {
      if (fscanf(f,"%ld",ts) == 1) {
	rc = 0;
      }
      fclose(f);
    } else {
      TRACE(hv_get_timestamp,("couldn't open timestamp file, switching to stat mode"));
      statmode = 1;
      rc = hv_get_timestamp(tspath,ts);
    }
  }
  TRACE_LEAVE(hv_get_timestamp,("rc=%d, ts=%ld",rc,*ts));
  return rc;
}

static int hv_get_ull(const char *ullpath, unsigned long long *ull)
{
  int    rc;
  FILE * f;
  
  TRACE_ENTER(hv_get_ull,("name=%s",ullpath));
  rc = -1;
  f = fopen(ullpath,"r");
  if (f) {
    if (fscanf(f,"%Lu",ull) == 1) {
      rc = 0;
    }
    fclose(f);
  }
  TRACE_LEAVE(hv_get_ull,("rc=%d, ull=%llu",rc,*ull));
  return rc;
}

static int hv_get_string(const char *strpath, char *str, size_t slen)
{
  int    rc;
  char   *np;
  FILE * f;
  
  TRACE_ENTER(hv_get_str,("name=%s",strpath));
  rc = -1;
  f = fopen(strpath,"r");
  if (f) {
    if (fgets(str,slen,f)) {
      np = strchr(str,'\n');
      if (np) {
	*np = 0;
      }
      rc = 0;
    }
    fclose(f);
  }
  TRACE_LEAVE(hv_get_str,("rc=%d, str=%s",rc,str));
  return rc;
}

static int  hv_get_dirnames(const char *basedir, char ***dirnames)
{
  DIR           *d;
  struct dirent *de;
  int            num, totalnum;

  TRACE_ENTER(hv_get_dirnames,("basedir=%s",basedir));
  
  num = totalnum = 0; 
  *dirnames      = NULL;
  d = opendir(basedir);
  if (d) {
    for (de = readdir(d); de; de = readdir(d)) {
      if (de->d_name[0] == '.') {
	/* strip out hidden entriese */
	continue;
      }
      if (num > totalnum - 1) {
	totalnum += 10;
	*dirnames = realloc(*dirnames,totalnum*sizeof(char*));
	if (*dirnames == NULL) {
	  TRACE(hv_get_dirnames,("realloc failed"));
	  num = -1;
	  break;
	}
      }
      (*dirnames)[num++] = strdup(de->d_name);
      (*dirnames)[num] = NULL;
    }
    closedir(d);
  } else {
    num = -1;
  }
  
  TRACE_LEAVE(hv_get_dirnames,("num=%d",num));
  return num;
}

static void hv_release_dirnames(char **dirnames)
{
  char **cp;
  TRACE_ENTER(hv_release_dirnames,("dirnames=%p",dirnames));
  
  cp = dirnames;
  while(cp && *cp) {
    free(*cp++);
  }
  free(dirnames);

  TRACE_LEAVE_V(hv_release_dirnames);
}

