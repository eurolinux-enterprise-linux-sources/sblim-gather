/*
 * $Id: merrno.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Error handling support
 *
 */

#include "merrno.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

static pthread_key_t  m_errno_key;
static pthread_key_t  m_errnostr_key;
static pthread_once_t m_errno_once = PTHREAD_ONCE_INIT;

static void m_errno_term(void *m_errno)
{
  if (m_errno) {
    free(m_errno);
  }
}

static void m_errno_init()
{
  pthread_key_create(&m_errno_key,m_errno_term);
  pthread_key_create(&m_errnostr_key,m_errno_term);
}

int m_errno()
{
  int *_m_errno;

  pthread_once(&m_errno_once,m_errno_init);
  _m_errno = (int*)pthread_getspecific(m_errno_key);
  return _m_errno ? *_m_errno : -1;
}

void m_seterrno(int err)
{
  int *_m_errno;

  pthread_once(&m_errno_once,m_errno_init);
  _m_errno = (int*)pthread_getspecific(m_errno_key);
  if (_m_errno==NULL)  {
    _m_errno = malloc(sizeof(int));
    pthread_setspecific(m_errno_key,_m_errno);
  }
  *_m_errno = err;
}

char * m_strerror()
{
  char *_m_errnostr;

  pthread_once(&m_errno_once,m_errno_init);
  _m_errnostr = (char*)pthread_getspecific(m_errnostr_key);
  return _m_errnostr;
}

void m_setstrerror(const char *fmt,...)
{
  char   *_m_errnostr;
  va_list ap;

  pthread_once(&m_errno_once,m_errno_init);
  _m_errnostr = (char*)pthread_getspecific(m_errnostr_key);
  if (_m_errnostr==NULL)  {
    _m_errnostr = malloc(1000);
    _m_errnostr[999]=0; /* make sure its null terrminated */
    pthread_setspecific(m_errnostr_key,_m_errnostr);
  }
  va_start(ap,fmt);
  vsnprintf(_m_errnostr,999,fmt,ap);
  va_end(ap);
}

