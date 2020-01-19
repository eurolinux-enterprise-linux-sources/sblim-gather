#
# $Id: sblim-gather.rh.spec,v 1.6 2009/05/20 19:39:56 tyreld Exp $
#
# Package spec for sblim-gather
#

%define peg24 %{?rhel4:1}%{!?rhel4:0}

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Summary: SBLIM Performance Data Gatherer
Name: sblim-gather
Version: 2.1.1pre1
Release: 1.rh%{?rhel4:el4}
Group: Systems Management/Base
URL: http://www.sblim.org
License: EPL

Source0: http://prdownloads.sourceforge.net/sblim/%{name}-%{version}.tar.bz2

%if %{peg24}
%define providerdir %{_libdir}/Pegasus/providers
BuildRequires: sblim-cmpi-devel
%else
%define providerdir %{_libdir}/cmpi
BuildRequires: tog-pegasus-devel >= 2.5
%endif
BuildRequires: sblim-cmpi-base-devel
BuildRequires: sysfsutils-devel
Requires: sysfsutils
Provides: sblim-gather-pluginz = %{version}

%Description
Standards Based Linux Instrumentation Performance Data Gatherer Base.
This package is containing the agents and control programs which can be
deployed stand-alone.

%Package provider
Summary: SBLIM Gatherer Provider
Group: Systems Management/Base
Requires: %{name} = %{version}
%if %{peg24}
%define providerdir %{_libdir}/Pegasus/providers
Requires: tog-pegasus >= 2.4
%else
%define providerdir %{_libdir}/cmpi
Requires: tog-pegasus >= 2.5
%endif
Requires: sblim-cmpi-base
Provides: sblim-gather-pluginz-provider = %{version}

%Description provider
This package is containing the CIM Providers for the SBLIM Gatherer.

%Package devel
Summary: SBLIM Gatherer Development Support
Group: Systems Management/Base
Requires: %{name} = %{version}

%Description devel
This package is needed to develop new plugins for the gatherer.

%Package test
Summary: SBLIM Gatherer Testcase Files
Group: Systems Management/Base
Requires: %{name}-provider = %{version}
Requires: sblim-testsuite

%Description test
SBLIM Gatherer Testcase Files for the SBLIM Testsuite

%prep

%setup -q

%build

%configure TESTSUITEDIR=%{_datadir}/sblim-testsuite \
	CIMSERVER=pegasus PROVIDERDIR=%{providerdir}

make %{?_smp_mflags}

%install

rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

# remove unused libtool files
rm -f $RPM_BUILD_ROOT/%{_libdir}/*a
rm -f $RPM_BUILD_ROOT/%{providerdir}/*a
rm -f $RPM_BUILD_ROOT/%{_libdir}/gather/*plug/*a

# remove non-devel .so's from libdir
find $RPM_BUILD_ROOT/%{_libdir} -maxdepth 1 -name "*.so" ! -name libgatherutil.so \
	-exec rm {} \;

%pre provider

%define SCHEMA 	%{_datadir}/%{name}/Linux_Metric.mof %{_datadir}/%{name}/Linux_IPProtocolEndpointMetric.mof %{_datadir}/%{name}/Linux_LocalFileSystemMetric.mof %{_datadir}/%{name}/Linux_NetworkPortMetric.mof %{_datadir}/%{name}/Linux_OperatingSystemMetric.mof %{_datadir}/%{name}/Linux_ProcessorMetric.mof %{_datadir}/%{name}/Linux_UnixProcessMetric.mof %{_datadir}/%{name}/Linux_XenMetric.mof %{_datadir}/%{name}/Linux_XenMetric.mof %{_datadir}/%{name}/Linux_zECKDMetric.mof %{_datadir}/%{name}/Linux_zCECMetric.mof %{_datadir}/%{name}/Linux_zLPARMetric.mof %{_datadir}/%{name}/Linux_zCHMetric.mof
%define REGISTRATION %{_datadir}/%{name}/Linux_IPProtocolEndpointMetric.registration %{_datadir}/%{name}/Linux_LocalFileSystemMetric.registration %{_datadir}/%{name}/Linux_Metric.registration %{_datadir}/%{name}/Linux_NetworkPortMetric.registration %{_datadir}/%{name}/Linux_OperatingSystemMetric.registration %{_datadir}/%{name}/Linux_ProcessorMetric.registration %{_datadir}/%{name}/Linux_UnixProcessMetric.registration %{_datadir}/%{name}/Linux_XenMetric.registration %{_datadir}/%{name}/Linux_zECKDMetric.registration %{_datadir}/%{name}/Linux_zCECMetric.registration %{_datadir}/%{name}/Linux_zLPARMetric.registration %{_datadir}/%{name}/Linux_zCHMetric.registration

# If upgrading, deregister old version
if [ $1 -gt 1 ]
then
  %{_datadir}/%{name}/provider-register.sh -t pegasus -d \
	-r %{REGISTRATION} -m %{SCHEMA} > /dev/null
fi

%post provider
# Register Schema and Provider - this is higly provider specific

%{_datadir}/%{name}/provider-register.sh -t pegasus \
	-r %{REGISTRATION} -m %{SCHEMA} > /dev/null

/sbin/ldconfig

%preun provider

# Deregister only if not upgrading 
if [ $1 -eq 0 ]
then
  %{_datadir}/%{name}/provider-register.sh -t pegasus -d \
	-r %{REGISTRATION} -m %{SCHEMA} > /dev/null
fi

%postun provider -p /sbin/ldconfig

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean

rm -rf $RPM_BUILD_ROOT

%files

%defattr(-,root,root) 
%config(noreplace) %{_sysconfdir}/*.conf
%docdir %{_datadir}/doc/%{name}-%{version}
%{_bindir}/*
%{_sbindir}/*
%{_datadir}/doc/%{name}-%{version}
%{_localstatedir}/run/gather
%{_libdir}/lib[^O]*.so.*
%{_libdir}/gather/mplug
%{_libdir}/gather/rplug

%files provider

%defattr(-,root,root) 
%{_libdir}/gather/cplug
%{_libdir}/libOSBase*.so.*
%{providerdir}
%{_datadir}/%{name}

%files devel

%defattr(-,root,root) 
%{_libdir}/lib[^O]*.so
%{_includedir}/gather

%files test

%defattr(-,root,root)
%{_datadir}/sblim-testsuite

%changelog

* Mon Jul  3 2006 Viktor Mihajlovski <mihajlov@dyn-9-152-143-50.boeblingen.de.ibm.com> - 2.1.1pre-1.{?rhel4:el4}
- Update for 2.1.1

* Fri Apr 21 2006 Viktor Mihajlovski <mihajlov@dyn-9-152-143-45.boeblingen.de.ibm.com> - 2.1.0-3.rh%{?rhel4:el4}
- Fixed file ownerships

* Wed Apr 11 2006 Viktor Mihajlovski <mihajlov@dyn-9-152-143-45.boeblingen.de.ibm.com> - 2.1.0-2.rh%{?rhel4:el4}
- Update for second build of RPM

* Fri Mar 31 2006 Viktor Mihajlovski <mihajlov@dyn-9-152-143-45.boeblingen.de.ibm.com> - 2.1.0-1.rh%{?rhel4:el4}
- Initial specfile for 2.1.0 for RH/Fedora

