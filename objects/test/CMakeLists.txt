kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kolftest_LIB_SRCS test.cpp )

kde4_automoc(${kolftest_LIB_SRCS})

kde4_add_library(kolftest SHARED ${kolftest_LIB_SRCS})

target_link_libraries(kolftest  ${KDE4_KDECORE_LIBS} kio kolf )

set_target_properties(kolftest PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} kolftest )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kolf FILES test.plugin )

kde4_footer()



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
