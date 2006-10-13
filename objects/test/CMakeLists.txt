



########### next target ###############

set(kolftest_LIB_SRCS test.cpp )

kde4_automoc(kolftest ${kolftest_LIB_SRCS})

kde4_add_library(kolftest SHARED ${kolftest_LIB_SRCS})

target_link_libraries(kolftest  ${KDE4_KIO_LIBS} kolf )

set_target_properties(kolftest PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kolftest  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES test.plugin  DESTINATION  ${DATA_INSTALL_DIR}/kolf )




#original Makefile.am contents follow:

#INCLUDES= $(all_includes)
#lib_LTLIBRARIES = libkolftest.la
#
#libkolftest_la_SOURCES = test.cpp
#
#libkolftest_la_LDFLAGS = $(all_libraries) $(LIB_KIO) -lkolf -module -avoid-version
#
#libkolftest_la_METASOURCES = AUTO
#
#noinst_HEADERS = test.h
#
#kolf_DATA = test.plugin
#kolfdir = $(kde_datadir)/kolf
