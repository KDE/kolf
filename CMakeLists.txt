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

kde4_add_library(kolfprivate SHARED ${kolf_LIB_SRCS})

target_link_libraries(kolfprivate  ${KDE4_KIO_LIBS} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_PHONON_LIBS} kdegames)

set_target_properties(kolfprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kolfprivate  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kolf_SRCS main.cpp)
set(kolf_icons
	pics/hi16-app-kolf.png
	pics/hi32-app-kolf.png
	pics/hi48-app-kolf.png
	pics/hi64-app-kolf.png
	pics/hi128-app-kolf.png
)
kde4_add_win32_app_icon(kolf_SRCS ${kolf_icons})
kde4_add_executable( kolf ${kolf_SRCS} )

target_link_libraries( kolf kolfprivate )
install(TARGETS kolf  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES kolf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kolfui.rc intro tutorial.kolf tutorial.kolfgame  DESTINATION  ${DATA_INSTALL_DIR}/kolf )
#install( FILES ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h floater.h slope.h  DESTINATION  ${INCLUDE_INSTALL_DIR}/kolf )

