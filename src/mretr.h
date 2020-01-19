/*
 * $Id: mretr.h,v 1.2 2009/05/20 19:39:55 tyreld Exp $
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
 * Description:  Threaded metric retriever.
 * This module offers services to process a metric block lists with
 * multiple threads in parallel.
 *
 */

#ifndef MRETR_H
#define MRETR_H

#include "mlist.h"

typedef void* MR_Handle;

MR_Handle MR_Init(ML_Head mhead, unsigned numthreads);
void MR_Finish(MR_Handle mrh);
void MR_Wakeup(MR_Handle mrh);

#endif
