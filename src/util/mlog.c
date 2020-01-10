/*
 * $Id: mlog.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: Michael Schuele <schuelem@de.ibm.com>
 *
 * Description: Metric Defintiona and Value data types.
 *
 */

const char *_mlog_id = "$Id: mlog.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $";

#include "mlog.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

void m_start_logging(const char *name)
{
  openlog(name,LOG_PID,LOG_DAEMON);
  setlogmask(LOG_UPTO(LOG_INFO));
}

void m_log(int priority, int errout, const char *fmt, ...)
{
  va_list ap;
  int priosysl;

  char buf[4096];

  switch (priority) {
  case M_DEBUG:
    priosysl=LOG_DEBUG;
    break;
  case M_INFO:
    priosysl=LOG_INFO;
    break;
  case M_ERROR:
  default:
    priosysl=LOG_ERR;
    break;
  }
  va_start(ap,fmt);
  
  vsnprintf(buf,4096,fmt,ap);
  syslog(priosysl,buf);

  if (errout) {
    vfprintf(stderr,fmt,ap);
  }
  va_end(ap);
}
