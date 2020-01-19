/*
 * $Id: mtracetest.c,v 1.3 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Trace Support Test
 *
 */

#undef NOTRACE

#include "mtrace.h"
#include <stdio.h>

int main()
{
  m_trace_setlevel(MTRACE_DETAILED);
  m_trace_enable(MTRACE_MASKALL);
  m_trace_setfile("test.trc");
  
  M_TRACE(MTRACE_ERROR,23,("Starting ..."));
  
  M_TRACE(MTRACE_FLOW,MTRACE_COMM,("Doing something %\n", "weird"));
  
  M_TRACE(MTRACE_ERROR,MTRACE_UTIL,("Stopping ..."));

  printf ("component string for %d is %s\n",MTRACE_COMM,m_trace_component(MTRACE_COMM));
  printf ("component id for %s is %d\n","gather",m_trace_compid("gather"));

  return 1;
}
