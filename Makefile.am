SUBDIRS = objects sounds courses pics graphics
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libkdegames -I$(top_srcdir)/libkdegames/highscore $(all_includes)

bin_PROGRAMS =
lib_LTLIBRARIES = libkolf.la
kdeinit_LTLIBRARIES = kolf.la

libkolf_la_SOURCES = \
	kolf.cpp game.cpp canvasitem.cpp ball.cpp newgame.cpp config.cpp \
	scoreboard.cpp editor.cpp pluginloader.cpp object.cpp vector.cpp \
	printdialogpage.cpp kcomboboxdialog.cpp kvolumecontrol.cpp \
	floater.cpp slope.cpp

libkolf_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 3:0:2
libkolf_la_LIBADD = $(LIB_KDEGAMES) $(LIB_KDEPRINT) $(LIB_KIO) -lartskde

# the installed header files
kolfinclude_HEADERS = \
	ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h \
	floater.h slope.h

kolfincludedir=$(includedir)/kolf


# kdeinited lib
kolf_la_SOURCES = main.cpp
kolf_la_LDFLAGS = $(KDE_RPATH) $(all_libraries) -module -avoid-version
kolf_la_LIBADD = libkolf.la


EXTRA_DIST = kolf.desktop

appdir = $(kde_datadir)/kolf
app_DATA = kolfui.rc intro tutorial.kolf tutorial.kolfgame

xdg_apps_DATA = kolf.desktop

applicationmimedir = $(kde_mimedir)/application
applicationmime_DATA = x-kourse.desktop x-kolf.desktop

magicdir = $(kde_confdir)/magic
magic_DATA = kolf.magic

METASOURCES = AUTO

messages: rc.cpp game.h floater.h slope.h
	$(XGETTEXT) *.cpp canvasitem.h game.h -o $(podir)/kolf.pot
