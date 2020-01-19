# $Id: Makefile.am,v 1.20 2012/08/15 02:13:58 tyreld Exp $
# ==================================================================
# (C) Copyright IBM Corp. 2005, 2009, 2011
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Eclipse Public License from
# http://www.opensource.org/licenses/eclipse-1.0.php
#
# Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
# Contributors: Dr. Gareth S. Bestor <bestorga@us.ibm.com>
#		          Oliver Benke <benke@de.ibm.com>
#               Tyrel Datwyler <tyreld@us.ibm.com>
# Description:  Automake input file for Gatherer Provider
# ==================================================================

# Start from an empty extra distribution file list
EXTRA_DIST=

# docdir defines where the documentation goes
#docdir=$(datadir)/doc/$(PACKAGE)-$(VERSION)

# plugin directories
cimplugindir=$(libdir)/gather/cplug

#
# Automake instructions for documentation
#
#doc_DATA=

# ADD EXTRA DOC FILES IF PRESENT
#EXTRA_DIST+=$(doc_DATA)

#doc_DATA+=doc/rpmbuild.txt
#EXTRA_DIST+=doc

# providerdir defines where provider libraries will be installed (@PROVIDERDIR@ is set by the configure script). 
providerdir = @PROVIDERDIR@

# Generally useful flags
AM_CPPFLAGS = -I $(srcdir)/.. -I $(srcdir)/../util -I $(srcdir)/../comms
AM_CFLAGS = -Wall
AM_LDFLAGS = -module -avoid-version -no-undefined

# Include files
noinst_HEADERS = OSBase_MetricUtil.h

if BASE_METRICS
BASE_PLUGS = libcimplugOperatingSystem.la \
             libcimplugLocalFileSystem.la \
             libcimplugProcessor.la \
             libcimplugUnixProcess.la \
             libcimplugIPProtocolEndpoint.la \
             libcimplugNetworkPort.la

BASE_MOFS = $(srcdir)/mof/Linux_IPProtocolEndpointMetric.mof \
            $(srcdir)/mof/Linux_LocalFileSystemMetric.mof \
            $(srcdir)/mof/Linux_NetworkPortMetric.mof \
            $(srcdir)/mof/Linux_OperatingSystemMetric.mof \
            $(srcdir)/mof/Linux_ProcessorMetric.mof \
            $(srcdir)/mof/Linux_UnixProcessMetric.mof \
	    $(srcdir)/mof/Linux_Storage.mof

BASE_REGS = $(srcdir)/mof/Linux_IPProtocolEndpointMetric.registration \
            $(srcdir)/mof/Linux_LocalFileSystemMetric.registration \
            $(srcdir)/mof/Linux_NetworkPortMetric.registration \
            $(srcdir)/mof/Linux_OperatingSystemMetric.registration \
            $(srcdir)/mof/Linux_ProcessorMetric.registration \
            $(srcdir)/mof/Linux_UnixProcessMetric.registration \
	    $(srcdir)/mof/Linux_Storage.registration
endif

if VIRT_METRICS
VIRT_PLUGS = libcimplugKvm.la \
             libcimplugXen.la

VIRT_MOFS = $(srcdir)/mof/Linux_XenMetric.mof \
            $(srcdir)/mof/Linux_KVMMetric.mof 

VIRT_REGS = $(srcdir)/mof/Linux_XenMetric.registration \
            $(srcdir)/mof/Linux_KVMMetric.registration 
endif

if Z_METRICS
Z_PLUGS = libcimplugzECKD.la \
          libcimplugzCEC.la \
          libcimplugzLPAR.la \
          libcimplugzCH.la

Z_MOFS = $(srcdir)/mof/Linux_zECKDMetric.mof \
         $(srcdir)/mof/Linux_zCECMetric.mof \
         $(srcdir)/mof/Linux_zLPARMetric.mof \
         $(srcdir)/mof/Linux_zCHMetric.mof

Z_REGS = $(srcdir)/mof/Linux_zECKDMetric.registration \
         $(srcdir)/mof/Linux_zCECMetric.registration \
         $(srcdir)/mof/Linux_zLPARMetric.registration \
         $(srcdir)/mof/Linux_zCHMetric.registration
endif

# Gather CIM plugin libraries
cimplugin_LTLIBRARIES = $(BASE_PLUGS) \
                        $(VIRT_PLUGS) \
                        $(Z_PLUGS)

libcimplugOperatingSystem_la_SOURCES=$(srcdir)/../plugin/cimplugOperatingSystem.c
libcimplugLocalFileSystem_la_SOURCES=$(srcdir)/../plugin/cimplugLocalFileSystem.c
libcimplugUnixProcess_la_SOURCES=$(srcdir)/../plugin/cimplugUnixProcess.c
libcimplugProcessor_la_SOURCES=$(srcdir)/../plugin/cimplugProcessor.c
libcimplugXen_la_SOURCES=$(srcdir)/../plugin/cimplugXen.c
libcimplugKvm_la_SOURCES=$(srcdir)/../plugin/cimplugKvm.c
libcimplugNetworkPort_la_SOURCES=$(srcdir)/../plugin/cimplugNetworkPort.c
libcimplugIPProtocolEndpoint_la_SOURCES=$(srcdir)/../plugin/cimplugIPProtocolEndpoint.c
libcimplugzECKD_la_SOURCES=$(srcdir)/../plugin/cimplugzECKD.c
libcimplugzCEC_la_SOURCES=$(srcdir)/../plugin/cimplugzCEC.c
libcimplugzCH_la_SOURCES=$(srcdir)/../plugin/cimplugzCH.c
libcimplugzLPAR_la_SOURCES=$(srcdir)/../plugin/cimplugzLPAR.c

# Gather CMPI provider utility libraries
lib_LTLIBRARIES = libOSBase_MetricUtil.la
libOSBase_MetricUtil_la_SOURCES=OSBase_MetricUtil.c
libOSBase_MetricUtil_la_CPPFLAGS= $(AM_CPPFLAGS) -DCIM_PLUGINDIR=\"$(cimplugindir)\"
libOSBase_MetricUtil_la_LIBADD=../librrepos.la 
libOSBase_MetricUtil_la_LDFLAGS=-lcmpiOSBase_Common

# Gather CMPI provider libraries
provider_LTLIBRARIES = libOSBase_MetricDefinitionProvider.la \
	libOSBase_MetricValueProvider.la \
    libOSBase_MetricServiceProvider.la \
    libOSBase_MetricServiceCapabilitiesProvider.la \
    libOSBase_MetricElementCapabilitiesProvider.la \
    libOSBase_MetricHostedServiceProvider.la \
    libOSBase_MetricServiceAffectsElementProvider.la \
    libOSBase_MetricRegisteredProfileProvider.la \
    libOSBase_MetricElementConformsToProfileProvider.la \
	libOSBase_MetricGathererProvider.la \
	libOSBase_MetricRepositoryServiceProvider.la \
	libOSBase_MetricForMEProvider.la \
	libOSBase_MetricDefForMEProvider.la \
	libOSBase_MetricInstanceProvider.la \
	libOSBase_MetricIndicationProvider.la \
	libOSBase_MetricLifeCycleProvider.la

libOSBase_MetricDefinitionProvider_la_SOURCES = OSBase_MetricDefinitionProvider.c
libOSBase_MetricDefinitionProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricValueProvider_la_SOURCES = OSBase_MetricValueProvider.c
libOSBase_MetricValueProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricServiceProvider_la_SOURCES = OSBase_MetricServiceProvider.c
libOSBase_MetricServiceProvider_la_LIBADD = @LIBSBLIMBASE@

libOSBase_MetricServiceCapabilitiesProvider_la_SOURCES = OSBase_MetricServiceCapabilitiesProvider.c

libOSBase_MetricElementCapabilitiesProvider_la_SOURCES = OSBase_MetricElementCapabilitiesProvider.c

libOSBase_MetricHostedServiceProvider_la_SOURCES = OSBase_MetricHostedServiceProvider.c

libOSBase_MetricServiceAffectsElementProvider_la_SOURCES = OSBase_MetricServiceAffectsElementProvider.c

libOSBase_MetricRegisteredProfileProvider_la_SOURCES = OSBase_MetricRegisteredProfileProvider.c

libOSBase_MetricElementConformsToProfileProvider_la_SOURCES = OSBase_MetricElementConformsToProfileProvider.c

libOSBase_MetricGathererProvider_la_SOURCES = OSBase_MetricGathererProvider.c
libOSBase_MetricGathererProvider_la_LIBADD = libOSBase_MetricUtil.la ../librgather.la @LIBSBLIMBASE@

libOSBase_MetricRepositoryServiceProvider_la_SOURCES = OSBase_MetricRepositoryServiceProvider.c
libOSBase_MetricRepositoryServiceProvider_la_LIBADD = libOSBase_MetricUtil.la @LIBSBLIMBASE@

libOSBase_MetricForMEProvider_la_SOURCES = OSBase_MetricForMEProvider.c
libOSBase_MetricForMEProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricDefForMEProvider_la_SOURCES = OSBase_MetricDefForMEProvider.c
libOSBase_MetricDefForMEProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricInstanceProvider_la_SOURCES = OSBase_MetricInstanceProvider.c
libOSBase_MetricInstanceProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricIndicationProvider_la_SOURCES = OSBase_MetricIndicationProvider.c
libOSBase_MetricIndicationProvider_la_LIBADD = libOSBase_MetricUtil.la

libOSBase_MetricLifeCycleProvider_la_SOURCES = OSBase_MetricLifeCycleProvider.c
libOSBase_MetricLifeCycleProvider_la_LIBADD = libOSBase_MetricUtil.la

#
# Automake instructions for ./mof subdir
#
PRE_MOFS= $(srcdir)/mof/Linux_Metric.mof

MOFS= $(BASE_MOFS) \
      $(VIRT_MOFS) \
      $(Z_MOFS)

INT_MOFS=	$(srcdir)/mof/Linux_MetricProfile.mof

PRE_REGS= $(srcdir)/mof/Linux_Metric.registration

REGS= $(BASE_REGS) \
      $(VIRT_REGS) \
      $(Z_REGS)
      
INT_REGS= 	$(srcdir)/mof/Linux_MetricProfile.registration

# Register the provider(s) and class definition(s) to the current CIM server/CIMOM
postinstall:
	sh $(srcdir)/provider-register.sh -t @CIMSERVER@ -r $(PRE_REGS) -m $(PRE_MOFS)
	sh $(srcdir)/provider-register.sh -t @CIMSERVER@ -r $(REGS) -m $(MOFS)
	sh $(srcdir)/provider-register.sh -t @CIMSERVER@ -n root/interop -r $(INT_REGS) -m $(INT_MOFS)

preuninstall:
	sh $(srcdir)/provider-register.sh -d -t @CIMSERVER@ -r $(REGS) -m $(MOFS)
	sh $(srcdir)/provider-register.sh -d -t @CIMSERVER@ -n root/interop -r $(INT_REGS) -m $(INT_MOFS)
	sh $(srcdir)/provider-register.sh -d -t @CIMSERVER@ -r $(PRE_REGS) -m $(PRE_MOFS)
	
dist-hook:
	test -d "$(distdir)" &&	rm -rf `find $(distdir) -type d -name CVS`

# We must explicity add all the schema files to the distribution package
pkgdata_DATA=$(PRE_MOFS) $(MOFS) $(INT_MOFS) $(PRE_REGS) $(REGS) $(INT_REGS)
pkgdata_SCRIPTS=provider-register.sh start_gathering.sh stop_gathering.sh

EXTRA_DIST+=$(pkgdata_DATA) $(pkgdata_SCRIPTS)

#
# Automake instructions for ./test subdir
#
EXTRA_DIST+=test indicationtest
if TESTSUITE
testsuitedir=@TESTSUITEDIR@
testsuitesystemdir=@TESTSUITEDIR@/system/linux
testsuitecimdir=@TESTSUITEDIR@/cim

testsuite_SCRIPTS=test/test-gather.sh
testsuitesystem_DATA= test/system/linux/Linux_IPProtocolEndpointMetric.system \
	test/system/linux/Linux_IPProtocolEndpointMetricValue.system \
	test/system/linux/Linux_LocalFileSystemMetric.system \
	test/system/linux/Linux_LocalFileSystemMetricValue.system \
	test/system/linux/Linux_MetricGatherer.system \
	test/system/linux/Linux_MetricRepositoryService.system \
	test/system/linux/Linux_NetworkPortMetric.system \
	test/system/linux/Linux_NetworkPortMetricValue.system \
	test/system/linux/Linux_OperatingSystemMetric.system \
	test/system/linux/Linux_OperatingSystemMetricValue.system \
	test/system/linux/Linux_ProcessorMetric.system \
	test/system/linux/Linux_ProcessorMetricValue.system \
	test/system/linux/Linux_UnixProcessMetric.system \
	test/system/linux/Linux_UnixProcessMetricValue.system

testsuitesystem_SCRIPTS=test/system/linux/gather-systemname.sh

testsuitecim_DATA= test/cim/Linux_IPProtocolEndpointMetric.cim \
	test/cim/Linux_IPProtocolEndpointMetricValue.cim \
	test/cim/Linux_LocalFileSystemMetric.cim \
	test/cim/Linux_LocalFileSystemMetricValue.cim \
	test/cim/Linux_MetricGatherer.cim \
	test/cim/Linux_MetricRepositoryService.cim \
	test/cim/Linux_NetworkPortMetric.cim \
	test/cim/Linux_NetworkPortMetricValue.cim \
	test/cim/Linux_OperatingSystemMetric.cim \
	test/cim/Linux_OperatingSystemMetricValue.cim \
	test/cim/Linux_ProcessorMetric.cim \
	test/cim/Linux_ProcessorMetricValue.cim \
	test/cim/Linux_UnixProcessMetric.cim \
	test/cim/Linux_UnixProcessMetricValue.cim 

runtest: install
	cd $(DESTDIR)$(TESTSUITEDIR) &&	./test-cmpi-base.sh

#doc_DATA+=README.TEST
endif
