/*
 * $Id: channelutil.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "channelutil.h"

#define CSS0 "/sys/devices/css0"

void parseCue(int cmg, char* cue, zChannelUtilization* result) {
	result->cmg = cmg;
	result->timestamp = *((unsigned int*)cue) & 0x00ffffff;
	result->cuiv = *cue;
	switch (cmg) {
		case 1:
			memcpy(&result->block1, cue, sizeof(zCueBlock1));
			break;
		case 2:
			memcpy(&result->block2, cue, sizeof(zCueBlock2));
			break;
		case 3:
			memcpy(&result->block3, cue, sizeof(zCueBlock3));
			break;
		default:
			result->timestamp = 0;
			result->cuiv = 0;
	}
}

//void releaseChannelUtilization(zChannelUtilization* cu) {
//}

void printChannelUtilization(zChannelUtilization* cu) {
	printf("CUE: {\n");
	switch (cu->cmg) {
		case 1:
			{
				printf("\tCUIV                        = %02hhx\n", cu->cuiv);
				printf("\ttimestamp                   =%11u\n", cu->timestamp);
				printf("\tcpc channel path busy time  =%11u\n", cu->block1.cpc_channel_path_busy_time);
				printf("\tlpar channel path busy time =%11u\n", cu->block1.lpar_channel_path_busy_time);
				printf("}\n");			
			}
			break;
		case 2:
			{
				printf("\tCUIV                    = %02hhx\n", cu->cuiv);
				printf("\ttimestamp               =%11u\n", cu->timestamp);
				printf("\tcpc bus cycles          =%11u\n", cu->block2.cpc_bus_cycles);
				printf("\tcpc channel work units  =%11u\n", cu->block2.cpc_channel_work_units);
				printf("\tcpc data units read     =%11u\n", cu->block2.cpc_data_units_read);
				printf("\tcpc data units written  =%11u\n", cu->block2.cpc_data_units_written);
				printf("\tlpar channel work units =%11u\n", cu->block2.lpar_channel_work_units);
				printf("\tlpar data units read    =%11u\n", cu->block2.lpar_data_units_read);
				printf("\tlpar data units written =%11u\n", cu->block2.lpar_data_units_written);
				printf("}\n");			
			}
			break;
		case 3:			
			{
				printf("\tCUIV                            = %02hhx\n", cu->cuiv);
				printf("\ttimestamp                       =%11u\n", cu->timestamp);
				printf("\tcpc data units sent             =%11u\n", cu->block3.cpc_data_units_sent);
				printf("\tcpc msg units sent              =%11u\n", cu->block3.cpc_msg_units_sent);
				printf("\tcpc unavailable rc buffers      =%11u\n", cu->block3.cpc_unavailable_receive_buffers);
				printf("\tlpar data units sent            =%11u\n", cu->block3.lpar_data_units_sent);
				printf("\tlpar msg units sent             =%11u\n", cu->block3.lpar_msg_units_sent);
				printf("\tlpar unavailable rc buffers     =%11u\n", cu->block3.lpar_unavailable_receive_buffers);
				printf("\tlpar unsuccessful send attempts =%11u\n", cu->block3.lpar_unsuccessful_attempts_to_sent);
				printf("}\n");			
			}
			break;
		default:
			printf("n/a\n}\n");			
	}
}

char* stringifyChannelUtilization(zChannelUtilization* cu) {
	const size_t maxresultsize = 255;
	char* result = calloc(maxresultsize, sizeof(char));
	if (result==NULL) return result;
	
	switch (cu->cmg) {
		case 1:
			{
				snprintf(result , maxresultsize, "1:%hhx:%u:%u:%u"
												, cu->cuiv
												, cu->timestamp
												, cu->block1.cpc_channel_path_busy_time
												, cu->block1.lpar_channel_path_busy_time);
			}
			break;
		case 2:
			{
				snprintf(result, maxresultsize, "2:%hhx:%u:%u:%u:%u:%u:%u:%u:%u"
												, cu->cuiv
												, cu->timestamp
												, cu->block2.cpc_bus_cycles
												, cu->block2.cpc_channel_work_units
												, cu->block2.cpc_data_units_read
												, cu->block2.cpc_data_units_written
												, cu->block2.lpar_channel_work_units
												, cu->block2.lpar_data_units_read
												, cu->block2.lpar_data_units_written);
			}
			break;
		case 3:			
			{
				snprintf(result, maxresultsize, "3:%hhx:%u:%u:%u:%u:%u:%u:%u:%u"
												, cu->cuiv
												, cu->timestamp
												, cu->block3.cpc_data_units_sent
												, cu->block3.cpc_msg_units_sent
												, cu->block3.cpc_unavailable_receive_buffers
												, cu->block3.lpar_data_units_sent
												, cu->block3.lpar_msg_units_sent
												, cu->block3.lpar_unavailable_receive_buffers
												, cu->block3.lpar_unsuccessful_attempts_to_sent);
			}
			break;
		default:
			snprintf(result, maxresultsize, "0");			
	}
	return result;
}

zChannelUtilization* destringifyChannelUtilization(char* cu, zChannelUtilization* result) {

	if (result==NULL) return NULL;
	
	if (cu==NULL || strlen(cu)<3) return NULL;

	result->cmg = atoi(cu);
	
	switch (result->cmg) {
		case 1:
			{
				sscanf(cu+2, "%hhx:%u:%u:%u"
											, &result->cuiv
											, &result->timestamp
											, &result->block1.cpc_channel_path_busy_time
											, &result->block1.lpar_channel_path_busy_time);
			}
			break;
		case 2:
			{
				sscanf(cu+2, "%hhx:%u:%u:%u:%u:%u:%u:%u:%u" 
												, &result->cuiv
												, &result->timestamp
												, &result->block2.cpc_bus_cycles
												, &result->block2.cpc_channel_work_units
												, &result->block2.cpc_data_units_read
												, &result->block2.cpc_data_units_written
												, &result->block2.lpar_channel_work_units
												, &result->block2.lpar_data_units_read
												, &result->block2.lpar_data_units_written);
			}
			break;
		case 3:
			{
				sscanf(cu+2, "%hhx:%u:%u:%u:%u:%u:%u:%u:%u" 
												, &result->cuiv
												, &result->timestamp
												, &result->block3.cpc_data_units_sent
												, &result->block3.cpc_msg_units_sent
												, &result->block3.cpc_unavailable_receive_buffers
												, &result->block3.lpar_data_units_sent
												, &result->block3.lpar_msg_units_sent
												, &result->block3.lpar_unavailable_receive_buffers
												, &result->block3.lpar_unsuccessful_attempts_to_sent);
			}
			break;
		default:
			return NULL;
	}
	return result;
}

void parseCmc(int cmg, char* cmc, zChannelMeasurementCharacteristics* result) {
	result->cmg = cmg;
	switch (cmg) {
		case 2:
			memcpy(&result->block2, cmc, sizeof(zCmcBlock2));
			break;
		case 3:
			memcpy(&result->block3, cmc, sizeof(zCmcBlock3));
			break;
	}
}

//void releaseChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc) {
//}

void printChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc) {
	printf("CMC: {\n");
	switch (cmc->cmg) {
		case 2:
			{
				printf("\tdata unit size          =%11u\n", cmc->block2.data_unit_size);
				printf("\tmaxi bus cycles         =%11u\n", cmc->block2.maximum_bus_cycles);
				printf("\tmax channel work units  =%11u\n", cmc->block2.maximum_channel_work_units);
				printf("\tmax write data units    =%11u\n", cmc->block2.maximum_write_data_units);
				printf("\tmax read data units     =%11u\n", cmc->block2.maximum_read_data_units);
				printf("}\n");			
			}
			break;
		case 3:			
			{
				printf("\tdata unit size          =%11u\n", cmc->block3.data_unit_size);
				printf("\tcpc data unit size      =%11u\n", cmc->block3.cpc_data_unit_size);
				printf("\tmsg unit size           =%11u\n", cmc->block3.msg_unit_size);
				printf("\tcpc msg unit size       =%11u\n", cmc->block3.cpc_msg_unit_size);
				printf("}\n");			
			}
			break;
		default:
			printf("n/a\n}\n");			
	}
}

char* stringifyChannelMeasurementCharacteristics(zChannelMeasurementCharacteristics* cmc) {
	const size_t maxresultsize = 255;
	char* result = calloc(maxresultsize, sizeof(char));
	if (result==NULL) return result;

	switch (cmc->cmg) {
		case 2:
			{
				snprintf(result, maxresultsize, "2:%u:%u:%u:%u:%u"
										, cmc->block2.data_unit_size
										, cmc->block2.maximum_bus_cycles
										, cmc->block2.maximum_channel_work_units
										, cmc->block2.maximum_write_data_units
										, cmc->block2.maximum_read_data_units);
			}
			break;
		case 3:			
			{
				snprintf(result, maxresultsize, "3:%u:%u:%u:%u"
										, cmc->block3.data_unit_size
										, cmc->block3.cpc_data_unit_size
										, cmc->block3.msg_unit_size
										, cmc->block3.cpc_msg_unit_size);
			}
			break;
		default:
			snprintf(result, maxresultsize, "0");			
	}
	return result;
}

zChannelMeasurementCharacteristics* destringifyChannelMeasurementCharacteristics(char* cmc, zChannelMeasurementCharacteristics* result) {

	if (result==NULL) return NULL;
	
	if (cmc==NULL || strlen(cmc)<3) return NULL;
	
	result->cmg = atoi(cmc);
	
	switch (result->cmg) {
		case 2:
			{
				sscanf(cmc+2, "%u:%u:%u:%u:%u"
										, &result->block2.data_unit_size
										, &result->block2.maximum_bus_cycles
										, &result->block2.maximum_channel_work_units
										, &result->block2.maximum_write_data_units
										, &result->block2.maximum_read_data_units);
			}
			break;
		case 3:
			{
				sscanf(cmc+2, "%u:%u:%u:%u"
										, &result->block3.data_unit_size
										, &result->block3.cpc_data_unit_size
										, &result->block3.msg_unit_size
										, &result->block3.cpc_msg_unit_size);
			}
			break;
		default:
			return NULL;
	}
	return result;
}

size_t iterateChpids(DeviceHandler handler, DeviceFilter filter) {
	size_t count = 0;
	sysfsw_Device* rootDevice = sysfsw_openDeviceTree(CSS0);
	if (rootDevice!=NULL) {
		sysfsw_DeviceList* deviceList = sysfsw_getDeviceList(rootDevice);
		if (deviceList!=NULL) {
			sysfsw_startDeviceList(deviceList);
			while (sysfsw_nextDevice(deviceList)) {
				sysfsw_Device* device = sysfsw_getDevice(deviceList);
				if (strncmp(sysfsw_getDeviceName(device), "chp", 3) == 0) {
					if (filter(device)) {
						handler(device, count++);
					}
				}
			}
		}
		sysfsw_closeDeviceTree(rootDevice);
	}
	return count;
}
