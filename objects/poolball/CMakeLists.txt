



########### next target ###############

set(kolfpoolball_LIB_SRCS poolball.cpp )

kde4_automoc(kolfpoolball ${kolfpoolball_LIB_SRCS})

kde4_add_library(kolfpoolball SHARED ${kolfpoolball_LIB_SRCS})

target_link_libraries(kolfpoolball   ${KDE4_KIO_LIBS} kolf )

set_target_properties(kolfpoolball PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kolfpoolball  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES poolball.plugin  DESTINATION  ${DATA_INSTALL_DIR}/kolf )



