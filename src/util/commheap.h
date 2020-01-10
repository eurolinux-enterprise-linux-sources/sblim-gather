/*
 * $Id: commheap.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Contributors: 
 *
 * Description: Memory Management functions for Communcation Functions
 */

#ifndef COMMHEAP_H
#define COMMHEAP_H

#include <stddef.h>

typedef void* COMMHEAP;

COMMHEAP ch_init();
int   ch_release(COMMHEAP ch);
void* ch_alloc(COMMHEAP ch,size_t sz);

#endif
