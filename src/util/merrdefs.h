/*
 * $Id: merrdefs.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * Description: Error Codes
 *
 */

#ifndef MERRDEFS_H
#define MERRDEFS_H

#define MERRNO_BASE         0
#define MERRNO_BASE_UTIL    (MERRNO_BASE + 100)
#define MERRNO_BASE_COMM    (MERRNO_BASE_UTIL + 100)
#define MERRNO_BASE_GATHER  (MERRNO_BASE_COMM + 100)
#define MERRNO_BASE_REPOS   (MERRNO_BASE_GATHER + 100)

#endif
