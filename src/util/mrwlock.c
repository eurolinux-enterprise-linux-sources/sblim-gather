/*
 * $Id: mrwlock.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Multiple Reader/Single Writer Locks
 */

#include "mrwlock.h"

int MReadLock(MRWLOCK *mrwl)
{
  if (mrwl && pthread_mutex_lock(&mrwl->mrw_mutex) == 0) {
    mrwl->mrw_rnum += 1;
    return pthread_mutex_unlock(&mrwl->mrw_mutex);
  } else {
    return -1;
  }
}

int MReadUnlock(MRWLOCK *mrwl)
{
  if (mrwl && pthread_mutex_lock(&mrwl->mrw_mutex) == 0) {
    mrwl->mrw_rnum -= 1;
    if (mrwl->mrw_rnum == 0)
      pthread_cond_broadcast(&mrwl->mrw_cond);
    return pthread_mutex_unlock(&mrwl->mrw_mutex);
  } else {
    return -1;
  }
}

int MWriteLock(MRWLOCK *mrwl)
{
  if (mrwl && pthread_mutex_lock(&mrwl->mrw_mutex) == 0) {
    while (mrwl->mrw_rnum) 
      pthread_cond_wait(&mrwl->mrw_cond,&mrwl->mrw_mutex);
    return 0;
  } else {
    return -1;
  }
}

int MWriteUnlock(MRWLOCK *mrwl)
{
  if (mrwl && pthread_mutex_unlock(&mrwl->mrw_mutex) == 0) {
    return 0; 
  } else {
    return -1;
  }
}
