cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "21")
set(RELEASE_SERVICE_VERSION_MINOR "11")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kolf VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.15.0")
set (KF5_MIN_VERSION "5.85.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Gui Widgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Completion
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DocTools
    I18n
    KIO
    TextWidgets
    WidgetsAddons
    XmlGui
)

find_package(KF5KDEGames 4.9.0 REQUIRED)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX KOLF VERSION_HEADER kolf_version.h)

add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050F00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

add_subdirectory(sounds)
add_subdirectory(courses)
add_subdirectory(pics)
add_subdirectory(graphics)
add_subdirectory(doc)

add_library(kolfprivate STATIC)

# we have a static copy of Box2D in our source tree
target_include_directories(kolfprivate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

target_sources(kolfprivate PRIVATE
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

target_sources(kolfprivate PRIVATE
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
)

target_link_libraries(kolfprivate
    KF5KDEGames
    KF5::Crash
    KF5::Completion
    KF5::ConfigCore
    KF5::DBusAddons
    KF5::I18n
    KF5::JobWidgets
    KF5::KIOCore
    KF5::TextWidgets
    KF5::XmlGui
    Qt::Gui
    Qt::Widgets
)

########### next target ###############

add_executable(kolf)
target_sources(kolf PRIVATE main.cpp)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/*-apps-kolf.png")
ecm_add_app_icon(kolf ICONS ${ICONS_SRCS})

target_link_libraries( kolf kolfprivate)
install(TARGETS kolf  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.kolf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES kolfui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kolf)
install(FILES intro tutorial.kolf tutorial.kolfgame  DESTINATION  ${KDE_INSTALL_DATADIR}/kolf)
#install(FILES ball.h game.h statedb.h config.h canvasitem.h object.h rtti.h vector.h floater.h slope.h  DESTINATION  ${KDE_INSTALL_INCLUDEDIR}/kolf COMPONENT Devel)
install(FILES org.kde.kolf.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
