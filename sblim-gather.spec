%global sblim_testsuite_version 1.2.4
%global provider_dir %{_libdir}/cmpi
%global tog_pegasus_version 2:2.6.1-1
%define _hardened_build 1

Name:           sblim-gather
Version:        2.2.8
Release:        9%{?dist}
Summary:        SBLIM Gatherer

Group:          Applications/System
License:        EPL
URL:            http://sourceforge.net/projects/sblim/
Source0:        http://downloads.sourceforge.net/project/sblim/%{name}/%{version}/%{name}-%{version}.tar.bz2
Source1:        gather-config.h.prepend
Source2:        gather-config.h
Source3:        sblim-gather.tmpfiles
Source4:        missing-providers.tgz
Source5:        gatherer.service
Source6:        reposd.service

BuildRequires:  sblim-cmpi-devel
BuildRequires:  sblim-cmpi-base-devel
BuildRequires:  tog-pegasus-devel >= %{tog_pegasus_version}
BuildRequires:  libsysfs-devel
BuildRequires:  libvirt-devel
BuildRequires:  xmlto
# for missing providers
BuildRequires:  cmake
Patch1:         sblim-gather-2.2.7-missing_providers.patch
Patch2:         sblim-gather-2.2.7-typos.patch

# Patch3: fixes multilib conflicts, rhbz#1076428
Patch3:         sblim-gather-2.2.8-multilib.patch
# Patch4: backported from upstream, rhbz#1467038
Patch4:         sblim-gather-2.2.8-SIGFPE-during-calcluation-of-interval-metric.patch
# Patch5: use Pegasus root/interop instead of root/PG_Interop
Patch5:         sblim-gather-2.2.8-pegasus-interop.patch

Requires:       tog-pegasus >= %{tog_pegasus_version}
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

%description
Standards Based Linux Instrumentation for Manageability
Performance Data Gatherer Base.
This package contains the agents and control programs for gathering
and providing performance data.

%package        provider
Summary:        SBLIM Gatherer Provider
Group:          Applications/System
BuildRequires:  tog-pegasus-devel >= %{tog_pegasus_version}
Requires:       %{name} = %{version}-%{release}
Requires:       sblim-cmpi-base
Requires:       tog-pegasus

%description    provider
The CIM (Common Information Model) Providers for the
SBLIM (Standards Based Linux Instrumentation for Manageability)
Gatherer.

%package        devel
Summary:        SBLIM Gatherer Development Support
Group:          Development/Libraries
BuildRequires:  tog-pegasus-devel >= %{tog_pegasus_version}
Requires:       %{name} = %{version}-%{release}
Requires:       tog-pegasus

%description    devel
This package is needed to develop new plugins for the
SBLIM (Standards Based Linux Instrumentation for Manageability)
Gatherer.

%package        test
Summary:        SBLIM Gatherer Testcase Files
Group:          Applications/System
BuildRequires:  tog-pegasus-devel >= %{tog_pegasus_version}
Requires:       %{name}-provider = %{version}-%{release}
Requires:       sblim-testsuite
Requires:       tog-pegasus

%description    test
Gatherer Testcase Files for the
SBLIM (Standards Based Linux Instrumentation for Manageability)
Testsuite

%prep
%setup -q
# for missing providers
tar xfvz %{SOURCE4}
%patch1 -p1 -b .missing_providers
%patch2 -p1 -b .typos
%patch3 -p1 -b .multilib
%patch4 -p1 -b .SIGFPE-during-calcluation-of-interval-metric
%patch5 -p1 -b .pegasus-interop

%build
%ifarch s390 s390x ppc ppc64
export CFLAGS="$RPM_OPT_FLAGS -fsigned-char -fno-strict-aliasing -Wl,-z,relro,-z,now"
%else
export CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing -Wl,-z,relro,-z,now"
%endif
%configure TESTSUITEDIR=%{_datadir}/sblim-testsuite \
        CIMSERVER=pegasus \
%ifarch s390 s390x
        --enable-z \
%endif
        PROVIDERDIR=%{provider_dir}
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool
make %{?_smp_mflags}

# for missing providers
pushd missing-providers
  mkdir -p %{_target_platform}
  pushd %{_target_platform}
    %{cmake} ..
    make %{?_smp_mflags}
  popd
popd

%install
make install DESTDIR=$RPM_BUILD_ROOT
# remove unused libtool files
rm -f $RPM_BUILD_ROOT/%{_libdir}/*a
rm -f $RPM_BUILD_ROOT/%{provider_dir}/*a
rm -f $RPM_BUILD_ROOT/%{_libdir}/gather/*plug/*a
# Install a redirection so that the arch-specific autoconf stuff continues to
# work but doesn't create multilib conflicts.
cat %{SOURCE1} \
        $RPM_BUILD_ROOT/%{_includedir}/gather/gather-config.h > \
        $RPM_BUILD_ROOT/%{_includedir}/gather/gather-config-%{_arch}.h
chmod 644 $RPM_BUILD_ROOT/%{_includedir}/gather/gather-config.h
install -m644 %{SOURCE2} $RPM_BUILD_ROOT/%{_includedir}/gather/

mkdir -p $RPM_BUILD_ROOT/%{_tmpfilesdir}
install -p -D -m 644 %{SOURCE3} $RPM_BUILD_ROOT/%{_tmpfilesdir}/sblim-gather.conf

# shared libraries
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/ld.so.conf.d
echo "%{_libdir}/cmpi" > $RPM_BUILD_ROOT/%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf

# for missing providers
make install/fast DESTDIR=$RPM_BUILD_ROOT -C missing-providers/%{_target_platform}
mkdir -p $RPM_BUILD_ROOT/var/lib/gather

# remove init script, install service files
rm $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/gatherer
mkdir -p ${RPM_BUILD_ROOT}%{_unitdir}
install -p -m 644 %{SOURCE5} $RPM_BUILD_ROOT%{_unitdir}/gatherer.service
install -p -m 644 %{SOURCE6} $RPM_BUILD_ROOT%{_unitdir}/reposd.service

%files
%config(noreplace) %{_sysconfdir}/*.conf
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf
%{_unitdir}/gatherer.service
%{_unitdir}/reposd.service
%docdir %{_datadir}/doc/%{name}-%{version}
%{_bindir}/*
%{_sbindir}/*
%{_datadir}/doc/%{name}-%{version}
%{_tmpfilesdir}/sblim-gather.conf
%ghost /var/run/gather
%{_libdir}/lib[^O]*.so.*
%dir %{_libdir}/gather
%{_libdir}/gather/mplug
%{_libdir}/gather/rplug
%{_mandir}/*/*

%files provider
%{_libdir}/gather/cplug
%{_libdir}/libOSBase_MetricUtil.so
%{_libdir}/libOSBase*.so.*
%{_libdir}/cmpi
%{_datadir}/%{name}
%dir /var/lib/gather

%files devel
%{_libdir}/lib[^O]*.so
%{_includedir}/gather

%files test
%{_datadir}/sblim-testsuite/cim/Linux*
%{_datadir}/sblim-testsuite/system/linux/Linux*
%{_datadir}/sblim-testsuite/system/linux/gather-systemname.sh
%{_datadir}/sblim-testsuite/test-gather.sh

%global GATHER_1ST_SCHEMA %{_datadir}/%{name}/Linux_Metric.mof
%global GATHER_1ST_REGISTRATION %{_datadir}/%{name}/Linux_Metric.registration

%global G_GLOB_IGNORE */Linux_Metric.*


%global GATHER_SCHEMA %{_datadir}/%{name}/*.mof
%global GATHER_REGISTRATION %{_datadir}/%{name}/*.registration

%post
install -d -m 0755 -o root -g root /var/run/gather
/sbin/ldconfig
%systemd_post gatherer.service
%systemd_post reposd.service

%preun
%systemd_preun gatherer.service
%systemd_preun reposd.service
if [ $1 -eq 0 ]; then
  rm -rf /var/run/gather
  rm -rf /var/lib/gather
fi

%postun
/sbin/ldconfig
%systemd_postun_with_restart gatherer.service
%systemd_postun_with_restart reposd.service

%pre provider
if [ $1 -gt 1 ]
then
  %{_datadir}/%{name}/provider-register.sh -t pegasus -d \
        -r %{GATHER_REGISTRATION} -m %{GATHER_SCHEMA} &> /dev/null || :;
  # don't let registration failure when server not running fail upgrade!
fi

%post provider
/sbin/ldconfig
if [ $1 -ge 1 ]
then
  GLOBIGNORE=%{G_GLOB_IGNORE}
  %{_datadir}/%{name}/provider-register.sh -t pegasus -r \
     %{GATHER_1ST_REGISTRATION} -m %{GATHER_1ST_SCHEMA} &>/dev/null || :;
  %{_datadir}/%{name}/provider-register.sh -t pegasus \
        -r %{GATHER_REGISTRATION} -m %{GATHER_SCHEMA} &>/dev/null || :;
  # don't let registration failure when server not running fail install!
fi

%preun provider
# Deregister only if not upgrading 
if [ $1 -eq 0 ]
then
  %{_datadir}/%{name}/provider-register.sh -t pegasus -d \
        -r %{GATHER_REGISTRATION} -m %{GATHER_SCHEMA} &> /dev/null || :;
  # don't let registration failure when server not running fail erase!
fi

%postun provider -p /sbin/ldconfig

%changelog
* Fri Jun 15 2018 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-9
- Use Pegasus root/interop instead of root/PG_Interop
- Enable System Z specific providers for s390 and s390x architecture
  Resolves: #1545811

* Tue Jul 11 2017 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-8
- Fix reposd crashes with SIGFPE during calcluation of interval metric when there
  are duplicate data values in the repository
  Resolves: #1467038

* Mon Feb 22 2016 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-7
- Fix sblim-gather is installing files under /etc/tmpfiles.d/
  Resolves: #1180992
- Build with PIE and full RELRO
  Resolves: #1092566

* Mon Mar 24 2014 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-6
- Fix failing scriptlets when CIMOM is not running
  Related: #1076428

* Mon Mar 17 2014 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-5
- Fix multilib conflicts
  Resolves: #1076428

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 2.2.8-4
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 2.2.8-3
- Mass rebuild 2013-12-27

* Mon May 06 2013 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-2
- Add -fno-strict-aliasing
- Do not ship old init script, add systemd support

* Mon Mar 18 2013 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.8-1
- Update to sblim-gather-2.2.8

* Wed Feb 27 2013 Roman Rakus <rrakus@redhat.com> - 2.2.7-3
- Fixed a typo
- Added missing providers
- improved providers registration
- Fixed owning of filesystem's directories (man)

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.2.7-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Tue Dec 04 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.7-1
- Update to sblim-gather-2.2.7
- Add man page BuildRequires, ship man pages

* Thu Sep 06 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.6-2
- Fix issues found by fedora-review utility in the spec file

* Wed Aug 15 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.6-1
- Update to sblim-gather-2.2.6

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.2.5-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon Apr 30 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.5-1
- Update to sblim-gather-2.2.5

* Wed Jan 04 2012 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.4-1
- Update to sblim-gather-2.2.4

* Wed May 18 2011 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.3-1
- Update to sblim-gather-2.2.3

* Thu Mar 24 2011 Vitezlsav Crhonek <vcrhonek@redhat.com> - 2.2.2-3
- Use %%ghost for /var/run/gather
  Resolves: #656686

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.2.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Mon Feb  7 2011 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.2-1
- Update to sblim-gather-2.2.2

* Mon Jun  7 2010 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.1-2
- Fix broken dependency because of missing libOSBase_MetricUtil.so

* Wed Jun  2 2010 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.2.1-1
- Update to sblim-gather-2.2.1

* Tue Oct 13 2009 Vitezslav Crhonek <vcrhonek@redhat.com> - 2.1.9-1
- Initial support
