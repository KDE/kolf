kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kolf/sounds FILES wall.wav puddle.wav holeinone.wav holed.wav blackhole.wav blackholeputin.wav blackholeeject.wav hit.wav )

kde4_footer()



#original Makefile.am contents follow:

#sounddir = $(kde_datadir)/kolf/sounds
#sound_DATA = wall.wav puddle.wav holeinone.wav holed.wav blackhole.wav blackholeputin.wav blackholeeject.wav hit.wav
#
#EXTRA_DIST=$(sound_DATA)
