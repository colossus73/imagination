# Adapted from Geany's plugins/Makefile.am, thanks

plugindir = $(libdir)/imagination

plugins_includedir = $(includedir)/imagination/

bar_wipe_la_LDFLAGS = -module -avoid-version
barn_door_wipe_la_LDFLAGS = -module -avoid-version
bow_tie_wipe_la_LDFLAGS = -module -avoid-version
box_wipe_la_LDFLAGS = -module -avoid-version
clock_wipe_la_LDFLAGS = -module -avoid-version
diagonal_wipe_la_LDFLAGS = -module -avoid-version
ellipse_wipe_la_LDFLAGS = -module -avoid-version
misc_la_LDFLAGS = -module -avoid-version
four_box_wipe_la_LDFLAGS = -module -avoid-version
insert_la_LDFLAGS = -module -avoid-version
push_wipe_la_LDFLAGS = -module -avoid-version
wave_la_LDFLAGS = -module -avoid-version
misc_diagonal_wipe_la_LDFLAGS = -module -avoid-version
misc_shape_wipe_la_LDFLAGS = -module -avoid-version
rochade_la_LDFLAGS = -module -avoid-version

# Plugins to be installed
plugin_LTLIBRARIES = \
	bar_wipe.la \
	barn_door_wipe.la  \
	bow_tie_wipe.la \
	box_wipe.la \
	clock_wipe.la \
	diagonal_wipe.la \
	ellipse_wipe.la		\
	misc.la			\
	four_box_wipe.la \
	insert.la \
	push_wipe.la \
	wave.la \
	misc_diagonal_wipe.la \
	misc_shape_wipe.la \
	rochade.la

bar_wipe_la_SOURCES	= bar_wipe.c
barn_door_wipe_la_SOURCES = barn_door_wipe.c
bow_tie_wipe_la_SOURCES = bow_tie_wipe.c
box_wipe_la_SOURCES = box_wipe.c
clock_wipe_la_SOURCES = clock_wipe.c
diagonal_wipe_la_SOURCES = diagonal_wipe.c
ellipse_wipe_la_SOURCES = ellipse_wipe.c
misc_la_SOURCES = misc.c
four_box_wipe_la_SOURCES = four_box_wipe.c
insert_la_SOURCES = insert.c
push_wipe_la_SOURCES = push_wipe.c
wave_la_SOURCES = wave.c
misc_diagonal_wipe_la_SOURCES = misc_diagonal_wipe.c
misc_shape_wipe_la_SOURCES = misc_shape_wipe.c
rochade_la_SOURCES = rochade.c

# instead of linking against all in $(GTK_LIBS), we link only against needed libs
bar_wipe_la_LIBADD = $(PLUGINS_LIBS)
barn_door_wipe_la_LIBADD = $(PLUGINS_LIBS)
bow_tie_wipe_la_LIBADD = $(PLUGINS_LIBS)
box_wipe_la_LIBADD = $(PLUGINS_LIBS)
clock_wipe_la_LIBADD = $(PLUGINS_LIBS)
diagonal_wipe_la_LIBADD = $(PLUGINS_LIBS)
ellipse_wipe_la_LIBADD = $(PLUGINS_LIBS)
misc_la_LIBADD = $(PLUGINS_LIBS)
four_box_wipe_la_LIBADD = $(PLUGINS_LIBS)
insert_la_LIBADD = $(PLUGINS_LIBS)
push_wipe_la_LIBADD = $(PLUGINS_LIBS)
wave_la_LIBADD = $(PLUGINS_LIBS)
misc_diagonal_wipe_la_LIBADD = $(PLUGINS_LIBS)
misc_shape_wipe_la_LIBADD = $(PLUGINS_LIBS)
rochade_la_LIBADD = $(PLUGINS_LIBS)

AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	$(PLUGINS_CFLAGS)

