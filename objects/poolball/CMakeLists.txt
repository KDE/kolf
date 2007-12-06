



########### next target ###############

set(kolfpoolball_LIB_SRCS poolball.cpp )

kde4_add_library(kolfpoolball SHARED ${kolfpoolball_LIB_SRCS})

target_link_libraries(kolfpoolball   ${KDE4_KIO_LIBS} kolf )

set_target_properties(kolfpoolball PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kolfpoolball  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES poolball.plugin  DESTINATION  ${DATA_INSTALL_DIR}/kolf )



