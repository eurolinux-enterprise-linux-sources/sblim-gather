#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([sblim-gather],[2.2.8],[sblim-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([cimplug.h])
AC_CONFIG_HEADER([gather-config.h])
AM_INIT_AUTOMAKE

# Host Type
AC_CANONICAL_HOST
case $host_cpu in
     s390*) ARCHDEF="-D S390";;
     **)    ARCHDEF="-D GENERIC";;
esac


# By default build no static libs
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT
AM_PROG_CC_C_O

AC_CHECK_PROG(enable_man,xmlto,"yes","no")

# Features
AC_ARG_ENABLE(base, 
      [AS_HELP_STRING([--disable-base], 
      [disable building of BaseOS metric plugins.])], 
      [enable_base=$enableval],
      [enable_base="yes"])

AC_ARG_ENABLE(virt, 
      [AS_HELP_STRING([--disable-virt],
      [disable building of Virtualization metric plugins.])],
      [enable_virt=$enableval], 
      [enable_virt="yes"])

AC_ARG_ENABLE(z, 
      [AS_HELP_STRING([--enable-z],
      [enable building of zLinux metric plugins.])],
      [enable_z=$enableval], 
      [enable_z="no"])

AC_ARG_ENABLE(provider,
      [AS_HELP_STRING(--disable-provider,disable building the provider.)],
	   [enable_provider=$enableval],
	   [enable_provider="yes"])

AC_ARG_ENABLE(debug,
      [AS_HELP_STRING([--enable-debug], 
      [enable debug output.])])

# DECLARE ANY SPECIAL CUSTOM CONFIGURE COMMAND LINE OPTIONS HERE
AC_ARG_VAR([TESTSUITEDIR],[the directory where the SBLIM testsuite resides.])
AC_ARG_VAR([PROVIDERDIR],[the directory where the CMPI providers will be installed.])
AC_ARG_VAR([CIMSERVER],[the target CIM server (pegasus|sfcb|openwbem|sniacimom).])

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [ GATHERLIBS="-lpthread" ])
AC_CHECK_LIB([dl], [dlopen], [ GATHERLIBS="-ldl $GATHERLIBS" ])

if test "$enable_z" == "yes"; then
   AC_CHECK_LIB([sysfs], [sysfs_open_driver], [ ZHYPLIBS="-lsysfs" ], [AC_MSG_ERROR([Cannot find library -lsysfs. Is libsysfs installed?])])
fi
   
if test "$enable_virt" == "yes"; then
   AC_CHECK_HEADERS([libvirt/libvirt.h], [], [AC_MSG_ERROR([Cannot find header file <libvirt/libvirt.h>. Is libvirt-devel installed?])])
   AC_CHECK_LIB([virt], [virConnectOpen], [ VIRTLIBS="-lvirt" ], [AC_MSG_ERROR([Cannot find library -lvirt. Is libvirt installed?])])
fi

AC_SUBST(GATHERLIBS)
AC_SUBST(ZHYPLIBS)
AC_SUBST(VIRTLIBS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h memory.h mntent.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/file.h sys/socket.h sys/time.h sys/vfs.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_GETMNTENT
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit dup2 gethostbyname gethostname getmntent gettimeofday inet_ntoa memmove memset socket strcasecmp strchr strdup strerror strrchr strspn strstr uname])

# Local type checks
AC_CHECK_SIZEOF([MetricValue],,[
#include <stdio.h>
#include "$srcdir/metric.h"
])			          
AC_CHECK_SIZEOF([MetricValue32],,[
#include <stdio.h>
#include "$srcdir/metric.h"
])			          
AC_CHECK_SIZEOF([MetricValue64],,[
#include <stdio.h>
#include "$srcdir/metric.h"
])			          
AC_CHECK_SIZEOF(long)

if test $ac_cv_sizeof_MetricValue = 0
then
    AC_MSG_ERROR([Unsupported platform: sizeof(MetricValue) cannot be determined.])
fi

if test $ac_cv_sizeof_long = 8
then
  if test $ac_cv_sizeof_MetricValue != $ac_cv_sizeof_MetricValue64
  then
    AC_MSG_ERROR([Unsupported platform: sizeof(MetricValue) != sizeof(MetricValue64)])
  fi
elif test $ac_cv_sizeof_long = 4
then
  if test $ac_cv_sizeof_MetricValue != $ac_cv_sizeof_MetricValue32
  then
    AC_MSG_ERROR([Unsupported platform: sizeof(MetricValue) != sizeof(MetricValue32)])
  fi
else 
  AC_MSG_ERROR([Unsupported platform: sizeof(long) == $ac_cv_sizeof_long])
fi


if test $enable_provider = "yes"
then
# Check for required CMPI header files (defined in acinclude.m4)
CHECK_CMPI

# Check for required libraries (defined in acinclude.m4)
CHECK_PROVIDERDIR

# Check for CIM Server (defined in acinclude.m4)
CHECK_CIMSERVER

# Check for SBLIM Include (defined in acinclude.m4)
CHECK_SBLIM_BASE

PROVIDER_SUBDIR=provider
AC_SUBST(PROVIDER_SUBDIR)
AC_CONFIG_FILES([provider/Makefile])
fi

# Check for Linux System Style

if test -f /etc/redhat-release 
then
   INIT_STYLE=init-redhat
elif test -f /etc/SuSE-release
then 
   INIT_STYLE=init-suse
else
   INIT_STYLE=init-none
fi
AC_SUBST(INIT_STYLE)

# Check for SBLIM Test Suite (defined in acinclude.m4)
CHECK_TESTSUITE

AC_CONFIG_FILES([Makefile sblim-gather.spec gatherer.init-suse
		gatherer.init-redhat gatherer.init-none
                man/gatherd.8.xml.pre man/gatherctl.8.xml.pre
                man/gatherd.conf.5.xml.pre man/reposd.8.xml.pre
                man/reposctl.8.xml.pre man/reposd.conf.5.xml.pre
                man/reposdump.1.xml.pre])

AC_SUBST(ARCHDEF)

AM_CONDITIONAL(BASE_METRICS, [test "$enable_base" == "yes"])
AM_CONDITIONAL(VIRT_METRICS, [test "$enable_virt" == "yes"])
AM_CONDITIONAL(Z_METRICS, [test "$enable_z" == "yes"])
AM_CONDITIONAL(PROVIDER,[test "$enable_provider" == "yes"]) 
AM_CONDITIONAL(GENMAN,[test "$enable_man" == "yes"])

if test "$enable_debug" == "yes"
then
	CPPFLAGS="-g -DDEBUG $CPPFLAGS"
else
	enable_debug=no
	CPPFLAGS="-DNOTRACE $CPPFLAGS"
fi

# Display configuration options
echo "-------------------------------------------------------"
echo "Configuration for $PACKAGE complete."
echo ""
echo "The following configuration options have been selected:"
echo ""
echo "Enable BaseOS metrics: " $enable_base
echo "Enable Virtualization metrics: " $enable_virt
echo "Enable zLinux metrics: " $enable_z
echo ""
echo "Enable Provider: " $enable_provider
echo "Enable Debug: " $enable_debug
echo ""
echo "Build manpages (requires xmlto): " $enable_man
echo ""
if test $enable_provider == yes
then
echo "CIMSERVER: " $CIMSERVER
echo "PROVIDERDIR: " $PROVIDERDIR
if test x"$TESTSUITEDIR" == x; then
   TESTSUITE_SUPPORT=Disabled
else	
   TESTSUITE_SUPPORT=Enabled
fi
echo "Test Suite Support" $TESTSUITE_SUPPORT
if test x"$TESTSUITEDIR" != x; then
echo "TESTSUITEDIR " $TESTSUITEDIR
fi
fi
echo "CPPFLAGS:" $CPPFLAGS
echo "-------------------------------------------------------"

AC_OUTPUT

echo "You may now run make"
