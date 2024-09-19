dnl Process this file with autoconf to produce a configure script.

AC_INIT([imagination],[4.0],[https://github.com/colossus73/imagination/issues])
AC_CONFIG_SRCDIR([src/imagination.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC_C_O
AM_SILENT_RULES([yes])

AC_PROG_INSTALL
IT_PROG_INTLTOOL

AC_SEARCH_LIBS([sqrtf], [m], ,AC_MSG_ERROR([Unable to find math library]))

# for plugins
LT_PREREQ([2.2.0])
LT_INIT
LIBTOOL="$LIBTOOL --silent"

pkg_modules="gtk+-3.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])

libavcodec="libavcodec >= 57.107.100"
PKG_CHECK_MODULES(LIBAVCODEC, [$libavcodec])

libavutil="libavutil >= 55.78.100"
PKG_CHECK_MODULES(LIBAVUTIL, [$libavutil])

libavformat="libavformat >= 57.83.100"
PKG_CHECK_MODULES(LIBAVFORMAT, [$libavformat])

libswscale="libswscale >= 5.7"
PKG_CHECK_MODULES(LIBSWSCALE, [$libswscale])

plugins_modules="cairo >= 1.6 glib-2.0 > 2.18.0"
PKG_CHECK_MODULES(PLUGINS, [$plugins_modules])

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_SUBST(LIBAVCODEC_CFLAGS)
AC_SUBST(LIBAVCODEC_LIBS)

AC_SUBST(LIBAVFORMAT_CFLAGS)
AC_SUBST(LIBAVFORMAT_LIBS)

AC_SUBST(LIBAVUTIL_CFLAGS)
AC_SUBST(LIBAVUTIL_LIBS)

AC_SUBST(LIBSWSCALE_CFLAGS)
AC_SUBST(LIBSWSCALE_LIBS)

AC_SUBST(PLUGINS_CFLAGS)
AC_SUBST(PLUGINS_LIBS)

GETTEXT_PACKAGE=imagination

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AM_GLIB_GNU_GETTEXT

AC_PATH_PROG([XSLTPROC], [xsltproc], [yes])

AM_CONDITIONAL([ENABLE_XSLTPROC], [test x"$enable_xsltproc" = x"yes"])

dnl Add the languages which your application supports here.
ALL_LINGUAS="`sed -e '/^#/d' $srcdir/po/LINGUAS`"


GTK_VERSION=`$PKG_CONFIG --modversion gtk+-3.0`
LIBAVUTIL_VERSION=`$PKG_CONFIG --modversion libavutil`
LIBAVCODEC_VERSION=`$PKG_CONFIG --modversion libavcodec`
LIBAVFORMAT_VERSION=`$PKG_CONFIG --modversion libavformat`
LIBSWSCALE_VERSION=`$PKG_CONFIG --modversion libswscale`

AC_DEFINE(LIBAVUTIL_VERSION,   ["$LIBAVUTIL_VERSION"] , []  )
AC_DEFINE(LIBAVCODEC_VERSION,  ["$LIBAVCODEC_VERSION"] , [] )
AC_DEFINE(LIBAVFORMAT_VERSION, ["$LIBAVFORMAT_VERSION"] , [] )
AC_DEFINE(LIBSWSCALE_VERSION,  ["$LIBSWSCALE_VERSION"] , [] )

AC_CONFIG_FILES([
Makefile
src/Makefile
transitions/Makefile
doc/Makefile
doc/en/Makefile
doc/en/imagination.xml
doc/en/images/Makefile
doc/fr/Makefile
doc/fr/imagination.xml
doc/fr/images/Makefile
doc/es/Makefile
doc/es/imagination.xml
doc/es/images/Makefile
doc/pt_BR/Makefile
doc/pt_BR/imagination.xml
doc/pt_BR/images/Makefile
po/Makefile.in
icons/Makefile
])
AC_OUTPUT

echo ""
echo "Imagination configuration:"
echo "--------------------------"
echo ""
echo "Install in  :       ${prefix}/bin"
echo "GTK version :       ${GTK_VERSION}"
echo "LibAVcodec  :       ${LIBAVCODEC_VERSION}"
echo "LibAVutil   :       ${LIBAVUTIL_VERSION}"
echo "LibAVformat :       ${LIBAVFORMAT_VERSION}"
echo "Libswscale  :       ${LIBSWSCALE_VERSION}"

echo ""
