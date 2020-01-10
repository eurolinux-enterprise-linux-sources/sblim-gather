/*
 * $Id: channelutil.h,v 1.2 2009/05/20 19:39:56 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2006, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Alexander Wolf-Reber <a.wolf-reber@de.ibm.com>
 * Contributors: 
 *
 * Description:
 * system Z channel measurement interface
 *
 */

#ifndef CHANNELUTIL_H_
#define CHANNELUTIL_H_

#include <stdlib.h>
#include "sysfswrapper.h"

typedef struct {
	unsigned int cuiv_timestamp;
	unsigned int cpc_channel_path_busy_time;
	unsigned int lpar_channel_path_busy_time;
} zCueBlock1;

typedef struct { 
	unsigned int cuiv_timestamp;
	unsigned int cpc_bus_cycles;
	unsigned int cpc_channel_work_units;
	unsigned int lpar_channel_work_units;
	unsigned int cpc_data_units_written;
	unsigned int lpar_data_units_written;
	unsigned int cpc_data_units_read;
	unsigned int lpar_data_units_read;
} zCueBlock2;

typedef struct {
	unsigned int cuiv_timestamp;
	unsigned int lpar_msg_units_sent;
	unsigned int cpc_msg_units_sent;
	unsigned int lpar_unsuccessful_attempts_to_sent;
	unsigned int lpar_unavailable_receive_buffers;
	unsigned int cpc_unavailable_receive_buffers;
	unsigned int lpar_data_units_sent;
	unsigned int cpc_data_units_sent;
} zCueBlock3;

typedef struct {
	int cmg;
	unsigned int timestamp;
	unsigned char cuiv;
	union {
		zCueBlock1 block1;
		zCueBlock2 block2;
		zCueBlock3 block3;
	};
} zChannelUtilization;

void parseCue(int cmg, char* cue, zChannelUtilization* result);
//void releaseChannelUtilization(zChannelUtilization* cu);
void printChannelUtilization(zChannelUtilization* cu);
char* stringifyChannelUtilization(zChannelUtilization* cu);
zChannelUtilization* destringifyChannelUtilization(char* cu, zChannelUtilization*  result);

typedef struct { 
	unsigned int maximum_bus_cycles;
	unsigned int maximum_channel_work_units;
	unsigned int maximum_write_data_units;
	unsigned int maximum_read_data_units;
	unsigned int data_unit_size;
} zCmcBlock2;

typedef struct {
	unsigned int data_unit_size;
	unsigned int cpc_data_unit_size;
	unsigned int msg_unit_size;
	unsigned int cpc_msg_unit_size;
} zCmcBlock3;

typedef struct {
	int cmg;
	union {
		zCmcBlock2 block2;
		zCmcBlock3 block3;
	};
} zChannelMeasurementCharacteristics;

void parseCmc(int cmg, char* cmc, zChannelMeasurementCharacteristics* result);
//void releaseChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc);
void printChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc);
char* stringifyChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc);
zChannelMeasurementCharacteristics* destringifyChannelMeasurementCharacteristics(char* cmc, zChannelMeasurementCharacteristics* result);

typedef void (DeviceHandler)(sysfsw_Device*, size_t);
typedef int (DeviceFilter)(sysfsw_Device*);
size_t iterateChpids(DeviceHandler handler, DeviceFilter filter);

#endif /*CHANNELUTIL_H_*/
