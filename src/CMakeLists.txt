ecm_setup_version(${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KOLF
    VERSION_HEADER kolf_version.h
)

add_library(kolfprivate STATIC)

# we have a static copy of Box2D in our source tree
target_include_directories(kolfprivate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

target_sources(kolfprivate PRIVATE
    3rdparty/Box2D/Collision/b2BroadPhase.cpp
    3rdparty/Box2D/Collision/b2BroadPhase.h
    3rdparty/Box2D/Collision/b2CollideCircle.cpp
    3rdparty/Box2D/Collision/b2CollideEdge.cpp
    3rdparty/Box2D/Collision/b2CollidePolygon.cpp
    3rdparty/Box2D/Collision/b2Collision.cpp
    3rdparty/Box2D/Collision/b2Collision.h
    3rdparty/Box2D/Collision/b2Distance.cpp
    3rdparty/Box2D/Collision/b2Distance.h
    3rdparty/Box2D/Collision/b2DynamicTree.cpp
    3rdparty/Box2D/Collision/b2DynamicTree.h
    3rdparty/Box2D/Collision/b2TimeOfImpact.cpp
    3rdparty/Box2D/Collision/b2TimeOfImpact.h
    3rdparty/Box2D/Collision/Shapes/b2CircleShape.cpp
    3rdparty/Box2D/Collision/Shapes/b2CircleShape.h
    3rdparty/Box2D/Collision/Shapes/b2EdgeShape.cpp
    3rdparty/Box2D/Collision/Shapes/b2EdgeShape.h
    3rdparty/Box2D/Collision/Shapes/b2LoopShape.cpp
    3rdparty/Box2D/Collision/Shapes/b2LoopShape.h
    3rdparty/Box2D/Collision/Shapes/b2PolygonShape.cpp
    3rdparty/Box2D/Collision/Shapes/b2PolygonShape.h
    3rdparty/Box2D/Collision/Shapes/b2Shape.h
    3rdparty/Box2D/Common/b2BlockAllocator.cpp
    3rdparty/Box2D/Common/b2BlockAllocator.h
    3rdparty/Box2D/Common/b2GrowableStack.h
    3rdparty/Box2D/Common/b2Math.cpp
    3rdparty/Box2D/Common/b2Math.h
    3rdparty/Box2D/Common/b2Settings.cpp
    3rdparty/Box2D/Common/b2Settings.h
    3rdparty/Box2D/Common/b2StackAllocator.cpp
    3rdparty/Box2D/Common/b2StackAllocator.h
    3rdparty/Box2D/Dynamics/b2Body.cpp
    3rdparty/Box2D/Dynamics/b2Body.h
    3rdparty/Box2D/Dynamics/b2ContactManager.cpp
    3rdparty/Box2D/Dynamics/b2ContactManager.h
    3rdparty/Box2D/Dynamics/b2Fixture.cpp
    3rdparty/Box2D/Dynamics/b2Fixture.h
    3rdparty/Box2D/Dynamics/b2Island.cpp
    3rdparty/Box2D/Dynamics/b2Island.h
    3rdparty/Box2D/Dynamics/b2TimeStep.h
    3rdparty/Box2D/Dynamics/b2WorldCallbacks.cpp
    3rdparty/Box2D/Dynamics/b2WorldCallbacks.h
    3rdparty/Box2D/Dynamics/b2World.cpp
    3rdparty/Box2D/Dynamics/b2World.h
    3rdparty/Box2D/Dynamics/Contacts/b2CircleContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2CircleContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2Contact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2Contact.h
    3rdparty/Box2D/Dynamics/Contacts/b2ContactSolver.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2ContactSolver.h
    3rdparty/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2LoopAndCircleContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2LoopAndCircleContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2LoopAndPolygonContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2LoopAndPolygonContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.h
    3rdparty/Box2D/Dynamics/Contacts/b2PolygonContact.cpp
    3rdparty/Box2D/Dynamics/Contacts/b2PolygonContact.h
    3rdparty/Box2D/Dynamics/Joints/b2DistanceJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2DistanceJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2FrictionJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2FrictionJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2GearJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2GearJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2Joint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2Joint.h
    3rdparty/Box2D/Dynamics/Joints/b2LineJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2LineJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2MouseJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2MouseJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2PrismaticJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2PulleyJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2PulleyJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2RevoluteJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2RopeJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2RopeJoint.h
    3rdparty/Box2D/Dynamics/Joints/b2WeldJoint.cpp
    3rdparty/Box2D/Dynamics/Joints/b2WeldJoint.h
)

target_sources(kolfprivate PRIVATE
    ball.cpp
    ball.h
    canvasitem.cpp
    canvasitem.h
    config.cpp
    config.h
    editor.cpp
    editor.h
    game.cpp
    game.h
    itemfactory.cpp
    itemfactory.h
    kcomboboxdialog.cpp
    kcomboboxdialog.h
    kolf.cpp
    kolf.h
    landscape.cpp
    landscape.h
    newgame.cpp
    newgame.h
    objects.cpp
    objects.h
    obstacles.cpp
    obstacles.h
    overlay.cpp
    overlay.h
    scoreboard.cpp
    scoreboard.h
    shape.cpp
    shape.h
    tagaro/board.cpp
    tagaro/board.h
    tagaro/scene.cpp
    tagaro/scene.h
    tagaro/scene_p.h
    tagaro/spriteobjectitem.cpp
    tagaro/spriteobjectitem.h
    utils-animateditem.cpp
    utils-animateditem.h
    vector.h
)

target_link_libraries(kolfprivate
    KDEGames6
    KF6::Crash
    KF6::Completion
    KF6::ConfigCore
    KF6::DBusAddons
    KF6::I18n
    KF6::JobWidgets
    KF6::KIOCore
    KF6::TextWidgets
    KF6::XmlGui
    Qt6::Gui
    Qt6::Widgets
)

add_executable(kolf)

target_sources(kolf PRIVATE
    main.cpp
    kolf.qrc
)

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/pics/*-apps-kolf.png")
ecm_add_app_icon(kolf ICONS ${ICONS_SRCS})

target_link_libraries(kolf kolfprivate)

install(TARGETS kolf  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.kolf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kolf.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

#install(FILES ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h floater.h slope.h  DESTINATION  ${KDE_INSTALL_INCLUDEDIR}/kolf COMPONENT Devel)
