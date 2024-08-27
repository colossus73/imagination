# $Id$

SUBDIRS =								\
	images

TARGET_DIR = $(datadir)/doc/imagination/html/es
STYLESHEET = ../imagination.xsl
DOCUMENT = imagination.xml

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
GPATH = $(srcdir)

DOC_STAMPS = html-build.stamp

EXTRA_DIST = $(DOCUMENT)
CLEANFILES = $(DOC_STAMPS)

all-local: html-build.stamp

html-build.stamp: $(srcdir)/$(DOCUMENT) $(srcdir)/$(STYLESHEET)
	@echo "*** Building Spanish documentation ***"
	@-chmod -R u+w $(srcdir)
	rm -rf $(srcdir)/html
	mkdir $(srcdir)/html/
	$(XSLTPROC) --nonet -o $(srcdir)/html/ $(srcdir)/$(STYLESHEET) \
		$(srcdir)/$(DOCUMENT)
	touch html-build.stamp

maintainer-clean-local: clean
	(cd $(srcdir) && rm -rf html)

install-data-local:
	installfiles=`echo $(srcdir)/html/*`;				\
	if test "$$installfiles" = '$(srcdir)/html/*'; then		\
		echo "--- Nothing to install";				\
	else								\
		$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR);		\
		for file in $$installfiles; do				\
			echo "--- Installing "$$file;			\
			$(INSTALL_DATA) $$file $(DESTDIR)$(TARGET_DIR);	\
		done;							\
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(TARGET_DIR)/*

dist-check-xsltproc: all

dist-hook: dist-check-xsltproc dist-hook-local
	mkdir $(distdir)/html
	-cp $(srcdir)/html/* $(distdir)/html

.PHONY: dist-hook-local

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
