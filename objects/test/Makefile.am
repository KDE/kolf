INCLUDES= $(all_includes)
lib_LTLIBRARIES = libkolftest.la

libkolftest_la_SOURCES = test.cpp

libkolftest_la_LDFLAGS = $(all_libraries) $(LIB_KIO) -lkolf -module -avoid-version

libkolftest_la_METASOURCES = AUTO

noinst_HEADERS = test.h

data_DATA = test.plugin
datadir = $(kde_datadir)/kolf
