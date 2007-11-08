project(kolf)


add_subdirectory( objects ) 
add_subdirectory( sounds ) 
add_subdirectory( courses ) 
add_subdirectory( pics ) 
add_subdirectory( graphics ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames/highscore  )


########### next target ###############

set(kolf_LIB_SRCS 
   kolf.cpp 
   game.cpp 
   canvasitem.cpp 
   ball.cpp 
   newgame.cpp 
   config.cpp 
   scoreboard.cpp 
   editor.cpp 
   pluginloader.cpp 
   object.cpp 
   vector.cpp 
   printdialogpage.cpp 
   kcomboboxdialog.cpp 
   kolfsvgrenderer.cpp
   floater.cpp 
   slope.cpp )

kde4_add_library(kolflib SHARED ${kolf_LIB_SRCS})

target_link_libraries(kolflib  ${KDE4_KIO_LIBS} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_PHONON_LIBS} kdegames)

set_target_properties(kolflib PROPERTIES VERSION 1.2.0 SOVERSION 1 )
install(TARGETS kolflib 
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)


########### next target ###############

kde4_add_executable( kolf main.cpp )

target_link_libraries( kolf kolflib )
install(TARGETS kolf  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES kolf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kolfui.rc intro tutorial.kolf tutorial.kolfgame  DESTINATION  ${DATA_INSTALL_DIR}/kolf )
#install( FILES ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h floater.h slope.h  DESTINATION  ${INCLUDE_INSTALL_DIR}/kolf )

