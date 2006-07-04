



########### install files ###############

install( FILES Classic.kolf Hard.kolf Medium.kolf Easy.kolf ReallyEasy Practice Impossible USApro  DESTINATION  ${DATA_INSTALL_DIR}/kolf/courses )




#original Makefile.am contents follow:

#coursedir = $(kde_datadir)/kolf/courses
#course_DATA = Classic.kolf Hard.kolf Medium.kolf Easy.kolf ReallyEasy Practice Impossible USApro
#
#EXTRA_DIST=$(course_DATA)
