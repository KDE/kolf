SUBDIRS = objects sounds courses pics graphics
bin_PROGRAMS = kolf
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)

kolf_SOURCES = main.cpp kolf.cpp game.cpp canvasitem.cpp ball.cpp newgame.cpp config.cpp scoreboard.cpp editor.cpp pluginloader.cpp object.cpp vector.cpp printdialogpage.cpp
kolf_LDFLAGS = $(all_libraries) $(KDE_RPATH) -export-dynamic
kolf_LDADD = $(top_builddir)/libkdegames/libkdegames.la $(LIB_KDEPRINT) $(LIB_KIO) -lartskde

EXTRA_DIST = kolf.desktop

appdir = $(kde_datadir)/kolf
app_DATA = kolfui.rc intro tutorial.kolf tutorial.kolfgame

appsdir = $(kde_appsdir)/Games/Arcade
apps_DATA = kolf.desktop

applicationmimedir = $(kde_mimedir)/application
applicationmime_DATA = x-kourse.desktop x-kolf.desktop

magicdir = $(kde_confdir)/magic
magic_DATA = kolf.magic

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kolf.pot

install-data-local:
	rm -rf $(DESTDIR)$(kde_appsdir)/Games/kolf.desktop
