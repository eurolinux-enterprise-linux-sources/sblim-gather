/*
 * $Id: mcfgtest.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
#include <stdio.h>

int main()
{
  const char * allowed[] = {
    "great",
    "medium",
    "small",
    NULL
  };
  char cfgbuf[10];
  int  cfghandle;

  if ((cfghandle=set_configfile("./test.cfg",allowed))<0) {
    printf("Failed to open config file %s\n","./test.cfg");
  }
  if (get_configitem(cfghandle,"great",cfgbuf,sizeof(cfgbuf))==0) {
    printf("Config value for %s is %s\n","great",cfgbuf);
  } else {
    printf("Failed to get config value for %s\n","great");
  }
  if (get_configitem(cfghandle,"medium",cfgbuf,sizeof(cfgbuf))==0) {
    printf("Config value for %s is %s\n","medium",cfgbuf);
  } else {
    printf("Failed to get config value for %s\n","medium");
  }
  if (get_configitem(cfghandle,"small",cfgbuf,sizeof(cfgbuf))==0) {
    printf("Config value for %s is %s\n","small",cfgbuf);
  } else {
    printf("Failed to get config value for %s\n","small");
  }
  if (get_configitem(cfghandle,"huge",cfgbuf,sizeof(cfgbuf))==0) {
    printf("Config value for %s is %s\n","huge",cfgbuf);
  } else {
    printf("Failed to get config value for %s\n","huge");
  }
  if (get_configitem(cfghandle,"gReAt",cfgbuf,sizeof(cfgbuf))==0) {
    printf("Config value for %s is %s\n","gReAt",cfgbuf);
  } else {
    printf("Failed to get config value for %s\n","gReAt");
  }

  return 0;
}
