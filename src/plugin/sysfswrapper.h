#ifndef SYSFSWRAPPER_H_
#define SYSFSWRAPPER_H_

// forward declarations from libsysfs header
struct sysfs_root_device;
struct sysfs_device;
struct dlist;

typedef struct sysfs_driver sysfsw_Driver;

typedef struct sysfs_device sysfsw_Device;

typedef struct sysfs_attribute sysfsw_Attribute;

typedef struct dlist sysfsw_DeviceList;

/* Driver */
sysfsw_Driver* sysfsw_openDriver(const char* bus, const char* driver);

void sysfsw_closeDriver(sysfsw_Driver* driver);

sysfsw_DeviceList* sysfsw_readDeviceList(sysfsw_Driver* driver);

/* Device List */

unsigned long sysfsw_getDeviceCount(sysfsw_DeviceList* deviceList);

void sysfsw_startDeviceList(sysfsw_DeviceList* deviceList);

int sysfsw_nextDevice(sysfsw_DeviceList* deviceList);

sysfsw_Device* sysfsw_getDevice(sysfsw_DeviceList* deviceList);

/* Device */

sysfsw_Device* sysfsw_openDevice(const char *devicePath);

char* sysfsw_getDeviceName(sysfsw_Device* device);

char* sysfsw_getDevicePath(sysfsw_Device* device);

void sysfsw_closeDevice(sysfsw_Device* device);

sysfsw_Device* sysfsw_openDeviceTree(const char *devicePath);

sysfsw_DeviceList* sysfsw_getDeviceList(sysfsw_Device* device);

void sysfsw_closeDeviceTree(sysfsw_Device* device);

/* Attribute */

sysfsw_Attribute* sysfsw_getAttribute(sysfsw_Device* device, const char* name);

sysfsw_Attribute* sysfsw_openAttributePath(char* devicepath, const char* path);

sysfsw_Attribute* sysfsw_openAttribute(sysfsw_Device* device, const char* path);

void sysfsw_closeAttribute(sysfsw_Attribute* attribute);

char* sysfsw_getAttributeValue(sysfsw_Attribute* attribute);

#endif /*SYSFSWRAPPER_H_*/
