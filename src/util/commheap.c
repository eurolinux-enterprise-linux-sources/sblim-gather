/*
 * $Id: commheap.c,v 1.4 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2003, 2009
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
 * Description: Memory Management for Communication Functions.
 */

#include "commheap.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>


#define COMMHEAP_HUNKSIZE 2000*sizeof(long)

typedef struct _COMMHEAP_HUNK {
  struct _COMMHEAP_HUNK *ch_prev;
  void                  *ch_heapstart;
  size_t                 ch_remaining;
} COMMHEAP_HUNK;

struct _COMMHEAP_CTL {
  COMMHEAP_HUNK *cc_head;
};

typedef struct _COMMHEAP_CTL COMMHEAP_CTL;

static int ch_extend(COMMHEAP_CTL * cc, size_t sz);

COMMHEAP ch_init()
{
  COMMHEAP_CTL * cc = calloc(1,sizeof(COMMHEAP_CTL));
  ch_extend(cc,0); 
  return (COMMHEAP)cc;
}

void* ch_alloc(COMMHEAP ch, size_t sz)
{
  COMMHEAP_CTL *cc = (COMMHEAP_CTL*)ch;
  size_t        sizemod;
  size_t        realsize;
  void         *ret;
  
  sizemod = sz%sizeof(long);
  realsize = sizemod ? sz-sizemod+sizeof(long) : sz;
  if (cc->cc_head->ch_remaining < realsize) {
    if (ch_extend(cc,realsize)) {
      return NULL;
    }
  }
  ret = cc->cc_head->ch_heapstart;
  cc->cc_head->ch_heapstart += realsize;
  cc->cc_head->ch_remaining -= realsize;
  return ret;
}

int ch_release(COMMHEAP ch)
{
  COMMHEAP_CTL *cc = (COMMHEAP_CTL*)ch;
  if (cc) {
    COMMHEAP_HUNK *ch = cc->cc_head;
    while (ch) {
      cc->cc_head = ch->ch_prev;
      free (ch);
      ch=cc->cc_head;
    }
    free(cc);
    return 0;
  }
  return -1;
}

static int ch_extend(COMMHEAP_CTL * cc, size_t sz)
{
  COMMHEAP_HUNK *ch = cc->cc_head;
  size_t         realrealsize=sz>COMMHEAP_HUNKSIZE?sz:COMMHEAP_HUNKSIZE;
  
  cc->cc_head = malloc(realrealsize + sizeof(COMMHEAP_HUNK));
  if (cc->cc_head) {
    cc->cc_head->ch_prev = ch;
    cc->cc_head->ch_remaining = realrealsize;
    cc->cc_head->ch_heapstart = (void*)(cc->cc_head + 1);
    return 0;
  }
  return -1;
}
