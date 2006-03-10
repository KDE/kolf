kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kolf/courses FILES Classic.kolf Hard.kolf Medium.kolf Easy.kolf ReallyEasy Practice Impossible USApro )

kde4_footer()



#original Makefile.am contents follow:

#coursedir = $(kde_datadir)/kolf/courses
#course_DATA = Classic.kolf Hard.kolf Medium.kolf Easy.kolf ReallyEasy Practice Impossible USApro
#
#EXTRA_DIST=$(course_DATA)
