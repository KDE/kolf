project(kolf)


add_subdirectory( sounds ) 
add_subdirectory( courses ) 
add_subdirectory( pics ) 
add_subdirectory( graphics ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames/highscore  )

# we have a static copy of Box2D in our source tree
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

########### next target ###############

set(box2d_SRCS
        external/Box2D/Common/b2StackAllocator.cpp
        external/Box2D/Common/b2Settings.cpp
        external/Box2D/Common/b2Math.cpp
        external/Box2D/Common/b2BlockAllocator.cpp
        external/Box2D/Dynamics/b2Island.cpp
        external/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp
        external/Box2D/Dynamics/Contacts/b2LoopAndCircleContact.cpp
        external/Box2D/Dynamics/Contacts/b2CircleContact.cpp
        external/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp
        external/Box2D/Dynamics/Contacts/b2Contact.cpp
        external/Box2D/Dynamics/Contacts/b2LoopAndPolygonContact.cpp
        external/Box2D/Dynamics/Contacts/b2PolygonContact.cpp
        external/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp
        external/Box2D/Dynamics/Contacts/b2ContactSolver.cpp
        external/Box2D/Dynamics/b2WorldCallbacks.cpp
        external/Box2D/Dynamics/b2Body.cpp
        external/Box2D/Dynamics/b2ContactManager.cpp
        external/Box2D/Dynamics/Joints/b2MouseJoint.cpp
        external/Box2D/Dynamics/Joints/b2PulleyJoint.cpp
        external/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp
        external/Box2D/Dynamics/Joints/b2WeldJoint.cpp
        external/Box2D/Dynamics/Joints/b2Joint.cpp
        external/Box2D/Dynamics/Joints/b2FrictionJoint.cpp
        external/Box2D/Dynamics/Joints/b2LineJoint.cpp
        external/Box2D/Dynamics/Joints/b2RopeJoint.cpp
        external/Box2D/Dynamics/Joints/b2GearJoint.cpp
        external/Box2D/Dynamics/Joints/b2DistanceJoint.cpp
        external/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp
        external/Box2D/Dynamics/b2World.cpp
        external/Box2D/Dynamics/b2Fixture.cpp
        external/Box2D/Collision/b2DynamicTree.cpp
        external/Box2D/Collision/b2Distance.cpp
        external/Box2D/Collision/b2CollideCircle.cpp
        external/Box2D/Collision/Shapes/b2CircleShape.cpp
        external/Box2D/Collision/Shapes/b2EdgeShape.cpp
        external/Box2D/Collision/Shapes/b2PolygonShape.cpp
        external/Box2D/Collision/Shapes/b2LoopShape.cpp
        external/Box2D/Collision/b2BroadPhase.cpp
        external/Box2D/Collision/b2Collision.cpp
        external/Box2D/Collision/b2TimeOfImpact.cpp
        external/Box2D/Collision/b2CollidePolygon.cpp
        external/Box2D/Collision/b2CollideEdge.cpp
)


set(kolf_LIB_SRCS
   game.cpp
   kolf.cpp
   newgame.cpp
   canvasitem.cpp
   ball.cpp
   config.cpp
   scoreboard.cpp
   editor.cpp
   kcomboboxdialog.cpp
   itemfactory.cpp
   landscape.cpp
   objects.cpp
   obstacles.cpp
   overlay.cpp
   shape.cpp
   utils-animateditem.cpp
   tagaro/board.cpp
   tagaro/scene.cpp
   tagaro/spriteobjectitem.cpp
   ${box2d_SRCS}
)

kde4_add_library(kolfprivate SHARED ${kolf_LIB_SRCS})

target_link_libraries(kolfprivate  ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS} kdegames )

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


