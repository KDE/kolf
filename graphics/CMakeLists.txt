kde4_header()




########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kolf/pics/ FILES cup.png grass.png puddle.png sand.png )

kde4_footer()



#original Makefile.am contents follow:

#picdir = $(kde_datadir)/kolf/pics/
#pic_DATA = cup.png grass.png puddle.png sand.png
#
#EXTRA_DIST=$(pic_DATA)
