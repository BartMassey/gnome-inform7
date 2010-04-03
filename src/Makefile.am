SUBDIRS = inweb cBlorb inform6 ni osxcart

AM_CFLAGS = -Wall

# This line won't be necessary anymore in Automake 1.10a
pkglibexecdir = $(libexecdir)/$(PACKAGE)

bin_PROGRAMS = gnome-inform7
gnome_inform7_SOURCES = \
	actions.c actions.h \
	app.c app.h app-private.h \
	builder.c builder.h \
	colorscheme.c colorscheme.h \
	configfile.c configfile.h \
	document.c document.h document-private.h \
	document-search.c \
	error.c error.h \
	extension.c extension.h \
	file.c file.h \
	history.c history.h \
	html.c html.h \
	lang.c lang.h \
	newdialog.c newdialog.h \
	notepad.c \
	panel.c panel.h panel-private.h \
	prefs.c prefs.h \
	rtf.c rtf.h \
	source-view.c source-view.h \
	spawn.c spawn.h \
	story.c story.h story-private.h \
	story-source.c story-errors.c story-index.c story-settings.c \
	story-compile.c story-game.c \
	welcomedialog.c welcomedialog.h \
	main.c 
gnome_inform7_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LIBEXEC_DIR=\""$(pkglibexecdir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
gnome_inform7_CFLAGS = @INFORM7_CFLAGS@ $(AM_CFLAGS)
gnome_inform7_LDADD = @INFORM7_LIBS@ $(INTLLIBS) osxcart/libosxcart.a
gnome_inform7_LDFLAGS = \
	-Wl,--export-dynamic
