/*
 * $Id: mtrace.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 *
 * Description:
 * Trace Support based on the SBLIM OSBASE Providers
*/

#ifndef MTRACE_H
#define MTRACE_H

extern unsigned     mtrace_mask;
extern int          mtrace_level;
extern const char * mtrace_file;


/* Levels and Components */

#define MTRACE_ALL      9
#define MTRACE_FLOW     3
#define MTRACE_DETAILED 2
#define MTRACE_ERROR    1
#define MTRACE_NONE     0

#define MTRACE_UTIL     0x0001
#define MTRACE_COMM     0x0002
#define MTRACE_REPOS    0x0004
#define MTRACE_GATHER   0x0008
#define MTRACE_RREPOS   0x0010
#define MTRACE_RGATHER  0x0020

#define MTRACE_UTIL_S    "util"
#define MTRACE_COMM_S    "comm"  
#define MTRACE_REPOS_S   "repos" 
#define MTRACE_GATHER_S  "gather"
#define MTRACE_RGATHER_S "rgather"
#define MTRACE_RREPOS_S  "rrepos"

#define MTRACE_MASKALL  0xffff


#ifdef NOTRACE
#define M_TRACE(LEVEL,COMP,STR)
#else
#define M_TRACE(LEVEL,COMP,STR) \
  if ( ((LEVEL)<=mtrace_level) && ((LEVEL)>0) && ((COMP)&mtrace_mask)) \
  _m_trace((LEVEL),COMP,__FILE__,__LINE__,_m_format_trace STR)

char * _m_format_trace (const char * fmt, ...);
void _m_trace(int level, unsigned component, const char *filename, 
	      int lineno, const char *msg);

void m_trace(int level, unsigned component, const char * filename, 
	     int lineno, const char * fmt, ...);

void m_trace_setfile(const char * tracefile);
void m_trace_setlevel(int level);
void m_trace_enable(unsigned component);
void m_trace_disable(unsigned component);
unsigned m_trace_compid(const char * component_name);
const char * m_trace_component(unsigned component);

#endif

#endif
