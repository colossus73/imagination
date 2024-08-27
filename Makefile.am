## Process this file with automake to produce Makefile.in

SUBDIRS = icons src transitions po doc

EXTRA_DIST = \
	autogen.sh \
	imagination.desktop.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

ACLOCAL_AMFLAGS = -I m4

uninstall-local:
	rm -rf $(DESTDIR)$(pkgdatadir);

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

desktopdir = $(datadir)/applications
desktop_in_files = imagination.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA)

installcheck-local: tests/run_tests.sh $(wildcard tests/*.py)
	IMAGINATION=$(bindir)/imagination tests/run_tests.sh
