/*
 * $Id: metricVirt.h,v 1.8 2011/11/29 06:28:09 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2009, 2011
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Tyrel Datwyler  <tyreld@us.ibm.com>
 * Contributors: 
 *
 * Description:
 * Plugin helper API for collecting Virtualization Metrics via Libvirt
 *
 */

#ifndef METRICVIRT_H
#define METRICVIRT_H

#include <mplugin.h>

#define NO_HYP 0
#define XEN_HYP 1
#define KVM_HYP 2

#define MAX_DOMAINS 255

#define VIRT_SUCCESS 0
#define VIRT_NOUPD   1
#define VIRT_FAIL    -1

struct vdisk_type {
	char * source;
	char * target;
	unsigned long long read;
	unsigned long long write;
	unsigned long long capacity;
	struct vdisk_type * next;
};

struct node_statistics_type {
	size_t num_active_domains;
	size_t num_inactive_domains;
	size_t total_domains;
	unsigned long long total_memory;
	unsigned long long free_memory;
} node_statistics;

struct domain_statistics_type {
	unsigned int domain_id[MAX_DOMAINS];
	char * domain_name[MAX_DOMAINS];
	unsigned long long claimed_memory[MAX_DOMAINS];
	unsigned long long max_memory[MAX_DOMAINS];
	float cpu_time[MAX_DOMAINS];
	unsigned short vcpus[MAX_DOMAINS];
	unsigned char  state[MAX_DOMAINS];
	unsigned long long cpu_used[MAX_DOMAINS];
	unsigned long long cpu_ready[MAX_DOMAINS];
	struct vdisk_type * blkio[MAX_DOMAINS];
} domain_statistics;

int testHypervisor(int type);

MetricRetriever virtMetricRetrCPUTime;
MetricRetriever virtMetricRetrTotalCPUTime;
MetricRetriever virtMetricRetrActiveVirtualProcessors;
MetricRetriever virtMetricRetrInternalMemory;
MetricRetriever virtMetricRetrHostFreePhysicalMemory;
MetricRetriever virtMetricRetrVirtualSystemState;
MetricRetriever virtMetricRetrCPUUsedTimeCounter;
MetricRetriever virtMetricRetrCPUReadyTimeCounter;
MetricRetriever virtMetricRetrVirtualBlockIOStats;

#endif
