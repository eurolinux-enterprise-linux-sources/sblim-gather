#include "sysfswrapper.h"
#include <sysfs/libsysfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <memory.h>


sysfsw_Driver* sysfsw_openDriver(const char* bus, const char* driver) {
	
	sysfsw_Driver* result = sysfs_open_driver(bus, driver);
	
	if (result == NULL) {
		fprintf(stderr, "error opening driver %s,%s (%d)\n", bus, driver, errno);
	} else {
//		printf("driver %s,%s opened\n", bus, driver);
	}
	return result;
}

sysfsw_DeviceList* sysfsw_readDeviceList(sysfsw_Driver* driver) {
	
	sysfsw_DeviceList* result = sysfs_get_driver_devices(driver);
	
	if (result == NULL) {
		fprintf(stderr, "error opening device list (%d)\n", errno);
	} else {
//		printf("device list opened\n");
	}
	
	return result;
}

void sysfsw_closeDriver(sysfsw_Driver* driver) {
	sysfs_close_driver(driver);
}

void sysfsw_startDeviceList(sysfsw_DeviceList* deviceList) {
	dlist_start(deviceList);
}

int sysfsw_nextDevice(sysfsw_DeviceList* deviceList) {
	if (deviceList->marker->next!=NULL && deviceList->marker->next!=deviceList->head) {
		dlist_next(deviceList);
		return 1;
	}
	return 0;
}

unsigned long sysfsw_getDeviceCount(sysfsw_DeviceList* deviceList) {
	return deviceList->count;
}

sysfsw_Device* sysfsw_getDevice(sysfsw_DeviceList* deviceList) {
	sysfsw_Device* result = dlist_mark(deviceList);
	return result;
}

sysfsw_Device* sysfsw_openDevice(const char* devicePath) {
	return sysfs_open_device_path(devicePath);
}

void sysfsw_closeDevice(sysfsw_Device* device) {
	sysfs_close_device(device);
} 

sysfsw_Device* sysfsw_openDeviceTree(const char* devicePath) {
	return sysfs_open_device_tree(devicePath);
}

sysfsw_DeviceList* sysfsw_getDeviceList(sysfsw_Device* device) {
	return device->children;
}

void sysfsw_closeDeviceTree(sysfsw_Device* device) {
	sysfs_close_device_tree(device);
} 

sysfsw_Attribute* sysfsw_openAttributePath(char* devicepath, const char* path) {
	char* attrpath = malloc(SYSFS_PATH_MAX+1);
	if (attrpath==NULL) return NULL;
	
	strncpy(attrpath, devicepath, SYSFS_PATH_MAX);
	strncat(attrpath, "/", SYSFS_PATH_MAX-strlen(attrpath));
	strncat(attrpath, path, SYSFS_PATH_MAX-strlen(attrpath));
	
	sysfsw_Attribute* attribute = sysfs_open_attribute(attrpath);
	sysfs_read_attribute(attribute);
	free(attrpath);
	return attribute;
}

sysfsw_Attribute* sysfsw_openAttribute(sysfsw_Device* device, const char* path) {
	return sysfsw_openAttributePath(device->path, path);
}

void sysfsw_closeAttribute(sysfsw_Attribute* attribute) {
	sysfs_close_attribute(attribute);
}

sysfsw_Attribute* sysfsw_getAttribute(sysfsw_Device* device, const char* name) {

	sysfsw_Attribute* result = sysfs_get_device_attr(device, name);
	if (result==NULL) {
		fprintf(stderr, "Get attribute failed %s (%d)", name, errno);
	}
	return result;
}

char* sysfsw_getDeviceName(sysfsw_Device* device) {
	return device!=NULL ? device->name : NULL;
}	

char* sysfsw_getDevicePath(sysfsw_Device* device) {
	return device!=NULL ? device->path : NULL;
}	

char* sysfsw_getAttributeValue(sysfsw_Attribute* attribute) {
	return attribute->value;
}
