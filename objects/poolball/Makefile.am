INCLUDES= $(all_includes)
lib_LTLIBRARIES = libkolfpoolball.la

libkolfpoolball_la_SOURCES = poolball.cpp

libkolfpoolball_la_LDFLAGS = $(all_libraries) $(LIB_KIO) -lkolf -module -avoid-version

libkolfpoolball_la_METASOURCES = AUTO

noinst_HEADERS = poolball.h

kolf_DATA = poolball.plugin
kolfdir = $(kde_datadir)/kolf
