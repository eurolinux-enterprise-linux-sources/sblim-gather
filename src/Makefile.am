# $Id: Makefile.am,v 1.30 2012/11/29 08:03:26 tyreld Exp $
# ==================================================================
# (C) Copyright IBM Corp. 2005, 2009
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
#               Oliver Benke <benke@de.ibm.com>
#               Tyrel Datwyler <tyreld@us.ibm.com>
# Description:  Automake input file for Files and Directories sample provider
# ==================================================================

AUTOMAKE_OPTIONS = no-dist-gzip dist-bzip2

SUBDIRS=. $(PROVIDER_SUBDIR)

# make sure that system z components are included
DISTCHECK_CONFIGURE_FLAGS = --enable-z

# Start from an empty extra distribution file list
EXTRA_DIST=

# gatherer run dir
gatherrundir=$(localstatedir)/run/gather

# docdir defines where the documentation goes
docdir=$(datadir)/doc/$(PACKAGE)-$(VERSION)

# initdir defines location of init scripts
initdir=$(sysconfdir)/init.d

# plugin directories
metricplugindir=$(libdir)/gather/mplug
reposplugindir=$(libdir)/gather/rplug

# development headers
pkgincludedir=$(includedir)/gather

#
# Automake instructions for documentation
#
doc_DATA=README AUTHORS COPYING README.TEST reposd2csv.pl

MANFILES=man/gatherd.8 man/gatherctl.8 man/gatherd.conf.5 \
	 man/reposd.8 man/reposctl.8 man/reposd.conf.5 \
	 man/reposdump.1

if BASE_METRICS
BASE_DOC=plugin/metricIPProtocolEndpoint.readme \
         plugin/metricNetworkPort.readme \
         plugin/metricLocalFileSystem.readme \
         plugin/metricProcessor.readme \
         plugin/metricOperatingSystem.readme \
         plugin/metricUnixProcess.readme

BASE_METRIC_LIBS=libmetricIPProtocolEndpoint.la \
                 libmetricNetworkPort.la \
                 libmetricLocalFileSystem.la \
                 libmetricProcessor.la \
                 libmetricOperatingSystem.la \
                 libmetricUnixProcess.la \
		 libmetricStorage.la

BASE_REPOS_LIBS=librepositoryIPProtocolEndpoint.la \
                librepositoryNetworkPort.la \
                librepositoryLocalFileSystem.la \
                librepositoryProcessor.la \
                librepositoryOperatingSystem.la \
                librepositoryUnixProcess.la \
		librepositoryStorage.la 
endif

if VIRT_METRICS
VIRT_DOC=plugin/metricKvm.readme \
         plugin/metricXen.readme

VIRT_METRIC_LIBS=libmetricKvm.la \
                 libmetricXen.la

VIRT_REPOS_LIBS=librepositoryKvm.la \
                librepositoryXen.la
endif

if Z_METRICS
Z_DOC=plugin/metriczECKD.readme \
      plugin/metriczCEC.readme \
      plugin/metriczLPAR.readme \
      plugin/metriczCH.readme

Z_METRIC_LIBS=libmetriczECKD.la \
              libmetriczCEC.la \
              libmetriczLPAR.la \
              libmetriczCH.la

Z_REPOS_LIBS=librepositoryzECKD.la \
             librepositoryzCEC.la \
             librepositoryzLPAR.la \
             librepositoryzCH.la

Z_LTLIBS=libsysfswrapper.la \
	 libhypfs.la \
	 liblparutil.la \
	 libchannelutil.la

Z_PROGS=hyptest \
	channelclt
endif

# ADD EXTRA DOC FILES IF PRESENT
doc_DATA+= $(BASE_DOC) \
           $(VIRT_DOC) \
           $(Z_DOC)

EXTRA_DIST+=$(doc_DATA)

#doc_DATA+=doc/rpmbuild.txt
#EXTRA_DIST+=doc

# Generally useful flags
AM_CPPFLAGS = -I $(srcdir)/util -I $(srcdir)/comms @ARCHDEF@
AM_CFLAGS = -Wall

PLUG_FLAGS = -no-undefined -module -avoid-version

# Include files
pkginclude_HEADERS = cimplug.h metric.h mplugin.h util/commutil.h \
	gather.h repos.h rrepos.h mrepos.h util/commheap.h
 
nodist_pkginclude_HEADERS = gather-config.h

noinst_HEADERS = gatherc.h marshal.h mlist.h mplugmgr.h \
	rplugin.h rreg.h sforward.h \
	mplugin.h mreg.h mretr.h \
	rgather.h rplugmgr.h slisten.h \
	util/gathercfg.h util/merrdefs.h \
	util/mlog.h util/mtrace.h util/mcfg.h \
	util/merrno.h util/mrwlock.h util/reposcfg.h util/dirutil.h \
	comms/mcclt.h comms/mcdefs.h comms/mcserv.h comms/rcclt.h \
	comms/rcserv.h \
	plugin/channelutil.h plugin/hypfs.h plugin/metricVirt.h

# Gather support utility libraries
lib_LTLIBRARIES = libgatherutil.la libmcserv.la librcserv.la \
	librrepos.la librepos.la libgather.la librgather.la \
	$(Z_LTLIBS)

libgatherutil_la_SOURCES = util/mrwlock.c util/commheap.c util/commutil.c \
	util/gatherutil.c util/mcfg.c util/gathercfg.c util/reposcfg.c \
	util/mlog.c util/mtrace.c util/merrno.c util/dirutil.c
libgatherutil_la_CPPFLAGS = $(AM_CPPFLAGS) -DGATHER_CONFDIR=\"$(sysconfdir)\"
libgatherutil_la_LIBADD = $(GATHERLIBS)

libmcserv_la_SOURCES = comms/mcclt_unix.c comms/mcserv_unix.c
libmcserv_la_CPPFLAGS = $(AM_CPPFLAGS) -DGATHER_RUNDIR=\"$(gatherrundir)\" 
libmcserv_la_LIBADD = libgatherutil.la

librcserv_la_SOURCES = comms/rcclt_ip.c comms/rcserv_ip.c
librcserv_la_LIBADD = libgatherutil.la

librrepos_la_SOURCES = rrepos.c marshal.c slisten.c
librrepos_la_LIBADD = libgatherutil.la libmcserv.la librcserv.la

librepos_la_SOURCES = repos.c mreposl.c rplugmgr.c rreg.c marshal.c
librepos_la_CPPFLAGS = $(AM_CPPFLAGS) -DREPOS_PLUGINDIR=\"$(reposplugindir)\"
librepos_la_LIBADD = libgatherutil.la

librgather_la_SOURCES = rgather.c
librgather_la_LIBADD = libgatherutil.la libmcserv.la

libgather_la_SOURCES = mlist.c mretr.c mplugmgr.c mreg.c mreposp.c gather.c
libgather_la_CPPFLAGS = $(AM_CPPFLAGS) -DGATHER_PLUGINDIR=\"$(metricplugindir)\"
libgather_la_LIBADD = librrepos.la libgatherutil.la

# Plugin Utility 
libsysfswrapper_la_SOURCES = plugin/sysfswrapper.c plugin/sysfswrapper.h
libsysfswrapper_la_LDFLAGS = $(ZHYPLIBS)

libhypfs_la_SOURCES = plugin/hypfs.c plugin/hypfs.h

liblparutil_la_SOURCES = plugin/lparutil.c plugin/lparutil.h
liblparutil_la_LIBADD = libhypfs.la

libchannelutil_la_SOURCES = plugin/channelutil.c plugin/channelutil.h
libchannelutil_la_LIBADD = libsysfswrapper.la
libchannelutil_la_LDFLAGS = $(ZHYPLIBS)

# Gather daemons
sbin_PROGRAMS = gatherd reposd

gatherd_SOURCES = gatherd.c
gatherd_LDADD = libgather.la libmcserv.la libgatherutil.la $(GATHERLIBS)

reposd_SOURCES = reposd.c sforward.c
reposd_LDADD = librepos.la libmcserv.la librcserv.la libgatherutil.la $(GATHERLIBS)

# Gather CLIs
bin_PROGRAMS = gatherctl reposctl reposdump

gatherctl_SOURCES = gatherctl.c
gatherctl_LDADD = librgather.la libgatherutil.la

reposctl_SOURCES = reposctl.c
reposctl_LDADD = librrepos.la libgatherutil.la

reposdump_SOURCES = reposdump.c
reposdump_CPPFLAGS = $(AM_CPPFLAGS) -DREPOS_PLUGINDIR=\"$(reposplugindir)\"
reposdump_LDADD = libgatherutil.la librrepos.la

# Gatherer Plugins
metricplugin_LTLIBRARIES = $(BASE_METRIC_LIBS) \
                           $(VIRT_METRIC_LIBS) \
                           $(Z_METRIC_LIBS)

libmetricOperatingSystem_la_SOURCES=plugin/metricOperatingSystem.c
libmetricOperatingSystem_la_LIBADD=libgatherutil.la
libmetricOperatingSystem_la_LDFLAGS=$(PLUG_FLAGS) -lm

libmetricLocalFileSystem_la_SOURCES=plugin/metricLocalFileSystem.c
libmetricLocalFileSystem_la_LIBADD=libgatherutil.la
libmetricLocalFileSystem_la_LDFLAGS=$(PLUG_FLAGS)

libmetricUnixProcess_la_SOURCES=plugin/metricUnixProcess.c
libmetricUnixProcess_la_LDFLAGS=$(PLUG_FLAGS)

libmetricProcessor_la_SOURCES=plugin/metricProcessor.c
libmetricProcessor_la_LIBADD=libgatherutil.la
libmetricProcessor_la_LDFLAGS=$(PLUG_FLAGS)

libmetricXen_la_SOURCES=plugin/metricXenV2.c plugin/metricVirt.c util/mlog.c
libmetricXen_la_LIBADD=libgatherutil.la 
libmetricXen_la_LDFLAGS=$(PLUG_FLAGS) $(VIRTLIBS)

libmetricKvm_la_SOURCES=plugin/metricKvm.c plugin/metricVirt.c util/mlog.c
libmetricKvm_la_LIBADD=libgatherutil.la 
libmetricKvm_la_LDFLAGS=$(PLUG_FLAGS) $(VIRTLIBS)

libmetricNetworkPort_la_SOURCES=plugin/metricNetworkPort.c
libmetricNetworkPort_la_LDFLAGS=$(PLUG_FLAGS) -lm

libmetricIPProtocolEndpoint_la_SOURCES=plugin/metricIPProtocolEndpoint.c
libmetricIPProtocolEndpoint_la_LDFLAGS=$(PLUG_FLAGS) -lm

libmetriczECKD_la_SOURCES=plugin/metriczECKD.c
libmetriczECKD_la_LIBADD=libsysfswrapper.la libgatherutil.la
libmetriczECKD_la_LDFLAGS=$(PLUG_FLAGS)

libmetriczLPAR_la_SOURCES=plugin/metriczLPAR.c
libmetriczLPAR_la_LIBADD=liblparutil.la libgatherutil.la
libmetriczLPAR_la_LDFLAGS=$(PLUG_FLAGS)

libmetriczCEC_la_SOURCES=plugin/metriczCEC.c
libmetriczCEC_la_LIBADD=liblparutil.la libgatherutil.la
libmetriczCEC_la_LDFLAGS=$(PLUG_FLAGS)

libmetriczCH_la_SOURCES=plugin/metriczCH.c
libmetriczCH_la_LIBADD=libchannelutil.la libgatherutil.la
libmetriczCH_la_LDFLAGS=$(PLUG_FLAGS)

libmetricStorage_la_SOURCES=plugin/metricStorage.c
libmetricStorage_la_LIBADD=libgatherutil.la
libmetricStorage_la_LDFLAGS=$(PLUG_FLAGS) -lm

reposplugin_LTLIBRARIES = $(BASE_REPOS_LIBS) \
                          $(VIRT_REPOS_LIBS) \
                          $(Z_REPOS_LIBS)

librepositoryOperatingSystem_la_SOURCES=plugin/repositoryOperatingSystem.c
librepositoryOperatingSystem_la_LIBADD=libgatherutil.la
librepositoryOperatingSystem_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryLocalFileSystem_la_SOURCES=plugin/repositoryLocalFileSystem.c
librepositoryLocalFileSystem_la_LIBADD=libgatherutil.la
librepositoryLocalFileSystem_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryUnixProcess_la_SOURCES=plugin/repositoryUnixProcess.c
librepositoryUnixProcess_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryProcessor_la_SOURCES=plugin/repositoryProcessor.c
librepositoryProcessor_la_LIBADD=libgatherutil.la
librepositoryProcessor_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryXen_la_SOURCES=plugin/repositoryXen.c
librepositoryXen_la_LIBADD=libgatherutil.la
librepositoryXen_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryKvm_la_SOURCES=plugin/repositoryKvm.c
librepositoryKvm_la_LIBADD=libgatherutil.la
librepositoryKvm_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryNetworkPort_la_SOURCES=plugin/repositoryNetworkPort.c
librepositoryNetworkPort_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryIPProtocolEndpoint_la_SOURCES=plugin/repositoryIPProtocolEndpoint.c
librepositoryIPProtocolEndpoint_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryzECKD_la_SOURCES=plugin/repositoryzECKD.c
librepositoryzECKD_la_LIBADD=libgatherutil.la
librepositoryzECKD_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryzLPAR_la_SOURCES=plugin/repositoryzLPAR.c
librepositoryzLPAR_la_LIBADD=libgatherutil.la
librepositoryzLPAR_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryzCEC_la_SOURCES=plugin/repositoryzCEC.c
librepositoryzCEC_la_LIBADD=libgatherutil.la
librepositoryzCEC_la_LDFLAGS=$(PLUG_FLAGS)

librepositoryzCH_la_SOURCES=plugin/repositoryzCH.c
librepositoryzCH_la_LIBADD=libchannelutil.la libgatherutil.la 
librepositoryzCH_la_LDFLAGS=$(PLUG_FLAGS) -lm

librepositoryStorage_la_SOURCES=plugin/repositoryStorage.c
librepositoryStorage_la_LIBADD=libgatherutil.la
librepositoryStorage_la_LDFLAGS=$(PLUG_FLAGS)

# Gather utility test programs (not installed)
noinst_PROGRAMS = mcfgtest mtracetest merrnotest mcstest mcctest \
	rcstest rcctest \
        $(Z_PROGS)

mcfgtest_SOURCES = util/mcfgtest.c
mcfgtest_LDADD = libgatherutil.la

mtracetest_SOURCES = util/mtracetest.c
mtracetest_LDADD = libgatherutil.la

merrnotest_SOURCES = util/merrnotest.c
merrnotest_LDADD = libgatherutil.la

mcstest_SOURCES = comms/mcstest.c
mcstest_LDADD = libmcserv.la

mcctest_SOURCES = comms/mcctest.c
mcctest_LDADD = libmcserv.la

rcstest_SOURCES = comms/rcstest.c
rcstest_LDADD = librcserv.la

rcctest_SOURCES = comms/rcctest.c
rcctest_LDADD = librcserv.la

EXTRA_DIST += util/test.cfg

# Plugin Test

hyptest_SOURCES = plugin/hyptest.c
hyptest_LDADD = libhypfs.la

channelclt_SOURCES = plugin/channelclt.c 
channelclt_LDADD = libchannelutil.la libsysfswrapper.la
channelclt_LDFLAGS = -lsysfs

# Gather init scripts
EXTRA_DIST += gatherer.init-suse.in gatherer.init-redhat.in \
				gatherer.init-none.in
if GENMAN
man_MANS=$(MANFILES)
endif

init_SCRIPTS=gatherer

gatherer: gatherer.$(INIT_STYLE)
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

SUFFIXES = .xml .xml.pre .1.xml .5.xml .8.xml .1 .5 .8

.xml.pre.xml:
	sed -e s?$$\{prefix\}?$(prefix)?g \
	-e s?$$\{exec_prefix\}?$(prefix)?g $< > $@

.1.xml.1:
	xmlto -o man man $<

.5.xml.5:
	xmlto -o man man $<

.8.xml.8:
	xmlto -o man man $<

# Gather config files
sysconf_DATA = gatherd.conf reposd.conf
EXTRA_DIST += $(sysconf_DATA)

# We must explicity add the RPM spec file to the distribution package
EXTRA_DIST+=$(PACKAGE).spec $(PACKAGE).rh.spec

EXTRA_DIST += samples

install-data-hook:
	$(mkdir_p) $(DESTDIR)$(gatherrundir)

postinstall:
	$(MAKE) -C $(PROVIDER_SUBDIR) postinstall

preuninstall:
	$(MAKE) -C $(PROVIDER_SUBDIR) preuninstall

clean-local:
	rm -f gatherer gatherer.init-redhat gatherer.init-suse gatherer.init-none \
	    man/*.8 man/*.5 man/*.1

dist-hook:
	test -d "$(distdir)" &&	rm -rf `find $(distdir) -type d -name CVS`
