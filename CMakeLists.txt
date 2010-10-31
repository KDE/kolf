project(kolf)


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
   printdialogpage.cpp 
   kcomboboxdialog.cpp 
   floater.cpp 
   slope.cpp
   itemfactory.cpp
   tagaro/board.cpp
   tagaro/scene.cpp
   tagaro/spriteobjectitem.cpp
)

kde4_add_library(kolfprivate SHARED ${kolf_LIB_SRCS})

target_link_libraries(kolfprivate  ${KDE4_KIO_LIBS} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_PHONON_LIBS} kdegames)

set_target_properties(kolfprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kolfprivate ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(kolf_SRCS main.cpp)

kde4_add_app_icon(kolf_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/hi*-app-kolf.png")
kde4_add_executable( kolf ${kolf_SRCS} )

target_link_libraries( kolf kolfprivate ${KDE4_KDEUI_LIBS})
install(TARGETS kolf  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS kolf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kolfui.rc intro tutorial.kolf tutorial.kolfgame  DESTINATION  ${DATA_INSTALL_DIR}/kolf )
#install( FILES ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h floater.h slope.h  DESTINATION  ${INCLUDE_INSTALL_DIR}/kolf COMPONENT Devel )


