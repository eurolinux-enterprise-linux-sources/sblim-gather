/*
 * $Id: channelclt.c,v 1.2 2009/05/20 19:39:56 tyreld Exp $
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
 * system Z channel measurement command line tool
 *
 */
 
#include "sysfswrapper.h"
#include "channelutil.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

const int true = (1==1);

void showDevice(sysfsw_Device* device, size_t pos) {
	
	int i;

	printf("\ndevice %s\n", sysfsw_getDeviceName(device));

	sysfsw_Attribute* cmgAttr = sysfsw_openAttribute(device, "cmg");
	char* cmgVal = sysfsw_getAttributeValue(cmgAttr);
	int cmg = atoi(cmgVal);
	printf("cmg = %d\n", cmg);
	sysfsw_closeAttribute(cmgAttr);

	sysfsw_Attribute* mcAttr = sysfsw_openAttribute(device, "measurement_chars");
	char* mcVal = sysfsw_getAttributeValue(mcAttr);

	printf("measurement_chars = ");
	for (i=0; i<20; ++i) {
		printf("%02hx ", mcVal[i]);
	}
	printf("\n");

	zChannelMeasurementCharacteristics cmc;
	parseCmc(cmg, mcVal, &cmc);
	sysfsw_closeAttribute(mcAttr);
	printChannelMeasurementCharacteristics(&cmc);
	printf("\n");

	sysfsw_Attribute* measAttr = sysfsw_openAttribute(device, "measurement");
	char* measVal = sysfsw_getAttributeValue(measAttr);

	printf("measurement = ");
	for (i=0; i<32; ++i) {
		printf("%02hx ", measVal[i]);
	}
	printf("\n");
	
	zChannelUtilization cu;
	parseCue(cmg, measVal, &cu);
	sysfsw_closeAttribute(measAttr);

	printChannelUtilization(&cu);
	printf("\n");
	
	/* test code for stringify/destringify */
/*
	char* cu_str = stringifyChannelUtilization(&cu);
	char* cmc_str = stringifyChannelMeasurementCharacteristics(&cmc);
	char* string = calloc(1, strlen(cu_str)+strlen(cmc_str)+2);
	strcpy( string, cu_str);	
	strcat( string, "!");
	strcat( string, cmc_str);
	
	printf("%s\n", string);
	
	char* sep = strstr(string, "!");
	if (sep==NULL) return;
	zChannelUtilization dcu;
	zChannelMeasurementCharacteristics dcmc;
	destringifyChannelUtilization(string, &dcu);
	destringifyChannelMeasurementCharacteristics(++sep, &dcmc);
	printChannelUtilization(&dcu);
	printChannelMeasurementCharacteristics(&dcmc);
	
	destringifyChannelMeasurementCharacteristics(sep, NULL);
	printf("NULL\n");
*/
}

int filterDevice(sysfsw_Device* device) {
	return true;
}

int main(int argc, char** argv)  {

	char * devicePath;
	
	if (argc<2) {
		printf("usage:\n\tchannelclt all | <chpid_path>\nexample:\n\tbtmtest /sys/devices/css0/chp0.41\n");
		exit(1);
	}
	
	devicePath = argv[1];

	printf("\nDisplay Channel Measurement Data\n");
	
	if (strncmp(devicePath, "all", 3) == 0) {
		size_t count = iterateChpids(showDevice, filterDevice);
		printf("%zu chpids shown\n", count);
	} else {
		printf("%s\n", devicePath);
		sysfsw_Device* device = sysfsw_openDevice(devicePath);
		if (device!=NULL) {
			showDevice(device, 0);
		sysfsw_closeDevice(device);
		}
	}
	
	return 0;
}
