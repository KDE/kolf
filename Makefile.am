SUBDIRS = objects sounds courses pics graphics
bin_PROGRAMS = kolf
INCLUDES = $(all_includes)

kolf_SOURCES = main.cpp kolf.cpp game.cpp canvasitem.cpp ball.cpp newgame.cpp config.cpp scoreboard.cpp editor.cpp pluginloader.cpp object.cpp
kolf_LDFLAGS = $(all_libraries) $(KDE_RPATH) -export-dynamic
kolf_LDADD = $(LIB_KDEUI) $(LIB_KDEPRINT) $(LIB_KIO) -lartskde

EXTRA_DIST = kolf.desktop

appdir = $(kde_datadir)/kolf
app_DATA = kolfui.rc tutorial.kolf

appsdir = $(kde_appsdir)/Games/Arcade
apps_DATA = kolf.desktop

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kolf.pot

install-data-local:
	rm -rf $(kde_appsdir)/Games/kolf.desktop
