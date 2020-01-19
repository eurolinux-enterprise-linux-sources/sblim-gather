/*
 * $Id: merrnotest.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Error handling test
 *
 */

#include "merrno.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void * f(void *v)
{
  m_seterrno(41);
  printf("errno is %d expected %d\n",41,m_errno());
  m_setstrerror("error for 41" );
  printf("errno is %s expected %s\n","error for 41",m_strerror());
  return NULL;
}

int main()
{
  pthread_t pt;
  pthread_create(&pt,NULL,f,NULL);
  pthread_create(&pt,NULL,f,NULL);
  
  m_seterrno(14);
  printf("errno is %d expected %d\n",14,m_errno());
  m_setstrerror("error for 14" );
  printf("errno is %s expected %s\n","error for 14",m_strerror());
  
  sleep(1);
  return 0;
}
