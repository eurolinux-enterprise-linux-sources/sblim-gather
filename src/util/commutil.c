/*
 * $Id: commutil.c,v 1.6 2009/05/20 19:39:56 tyreld Exp $
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
 * Author:       Heidi Neumann <heidineu@de.ibm.cim>
 * Contributors: Michael Schuele <schuelem@de.ibm.com>
 *
 * Description: Communcation Utility Functions
 */


#include <stdlib.h>
#include <commutil.h>
#include "gather-config.h"

#ifndef AIX
uint64_t htonll(uint64_t v)
{
#ifdef WORDS_BIGENDIAN
  return v;
#else
  return __bswap_64 (v);
#endif
}
#endif

#ifndef AIX
uint64_t ntohll(uint64_t v)
{
  return htonll(v);
}
#endif


/*
 * Note: Don't think this will work in all environments (s390,power)
 */
float htonf(float v) 
{
#ifdef WORDS_BIGENDIAN
  return v;
#else
  unsigned char *bp = (unsigned char *)&v;
  unsigned char *array;
  unsigned       i;

  array = malloc(sizeof(float));

  for (i=0; i<sizeof(float);i++) {
    array[i] = bp[sizeof(float)-1-i];
  }

  v = (*(float *) array);
  free(array);

  return v;
#endif 
}

float ntohf(float v) 
{
  return htonf(v);
}
