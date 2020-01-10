/*
 * $Id: hyptest.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Hypervisor FS Test Program
 *
 */

#include "hypfs.h"
#include <stdio.h>

static void print_system(HypSystem * hs);
static void print_processor(HypProcessor * hp);

int main(int argc, char *argv[])
{
  HypSystem * hs;

  hs = get_hypervisor_info(get_hypervisor_rootpath());
  
  if (hs) {
    print_system(hs);
    release_hypervisor_info(hs);
  }
  return 0;
}

static void print_system(HypSystem * hs)
{
  int    i;
  char * prefix = hs->hs_vtype == HYPFS_PHYSICAL ? "" : "   ";
  char   hdrc = hs->hs_vtype == HYPFS_PHYSICAL ? '=' : '-';
  int    hdrl = hs->hs_vtype == HYPFS_PHYSICAL ? 50 : 47;
  
  printf("%s",prefix);
  for (i=0;i<hdrl;i++) {
    printf("%c",hdrc);
  }
  printf("\n");
  printf("%s%s System %s\n",prefix,
	 hs->hs_vtype == HYPFS_PHYSICAL ? "Physical" : "Logical",
	 hs->hs_id);
  if (hs->hs_timestamp) {
    printf("%sSample Time: %s",prefix,ctime(&hs->hs_timestamp));
  }
  printf("%sContained PUs: %d\n",prefix,hs->hs_numcpu);
  for (i=0; i<hs->hs_numcpu;i++) {
    print_processor(hs->hs_cpu+i);
  }
  if (hs->hs_numsys != -1) {
    printf("%sContained Systems: %d\n",prefix,hs->hs_numsys);
    for (i=0; i<hs->hs_numsys;i++) {
      print_system(hs->hs_sys+i);
    }
  }
  printf("%s",prefix);
  for (i=0;i<hdrl;i++) {
    printf("%c",hdrc);
  }
  printf("\n");
}

static void print_processor(HypProcessor * hp)
{
  int    i;
  char * prefix = hp->hp_vtype == HYPFS_PHYSICAL ? "   " : "      ";
  char   hdrc = hp->hp_vtype == HYPFS_PHYSICAL ? ':' : '.';
  int    hdrl = hp->hp_vtype == HYPFS_PHYSICAL ? 47 : 44;
  
  printf("%s",prefix);
  for (i=0;i<hdrl;i++) {
    printf("%c",hdrc);
  }
  printf("\n");
  printf("%s%s Processor %s\n",prefix,
	 hp->hp_vtype == HYPFS_PHYSICAL ? "Physical" : "Logical",
	 hp->hp_id);
  printf("%sType Info: %s\n",prefix, hp->hp_info);
  if (hp->hp_mgmttime != -1LL) {
    printf("%sManagement Time: %llu\n",prefix,hp->hp_mgmttime);
  }
  if (hp->hp_cputime != -1LL) {
    printf("%sCPU Time: %llu\n",prefix,hp->hp_cputime);
  }
  if (hp->hp_onlinetime != -1LL) {
    printf("%sOnline Time: %llu\n",prefix,hp->hp_onlinetime);
  }
  printf("%s",prefix);
  for (i=0;i<hdrl;i++) {
    printf("%c",hdrc);
  }
  printf("\n");
}
