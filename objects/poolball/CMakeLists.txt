



########### next target ###############

set(kolfpoolball_LIB_SRCS poolball.cpp )

kde4_automoc(${kolfpoolball_LIB_SRCS})

kde4_add_library(kolfpoolball SHARED ${kolfpoolball_LIB_SRCS})

target_link_libraries(kolfpoolball  ${KDE4_KDECORE_LIBS} kio kolf )

set_target_properties(kolfpoolball PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} kolfpoolball )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kolf FILES poolball.plugin )




#original Makefile.am contents follow:

#INCLUDES= $(all_includes)
#lib_LTLIBRARIES = libkolfpoolball.la
#
#libkolfpoolball_la_SOURCES = poolball.cpp
#
#libkolfpoolball_la_LDFLAGS = $(all_libraries) $(LIB_KIO) -lkolf -module -avoid-version
#
#libkolfpoolball_la_METASOURCES = AUTO
#
#noinst_HEADERS = poolball.h
#
#kolf_DATA = poolball.plugin
#kolfdir = $(kde_datadir)/kolf
