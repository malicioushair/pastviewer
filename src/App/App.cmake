include(cmake/Helpers.cmake)

set(APP_MAJOR_VERSION 0)
set(APP_MINOR_VERSION 1)
set(APP_PATCH_VERSION 0)

find_package(glog REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Quick QuickLayouts QuickControls2 Location Positioning PositioningQuick REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)
include(ext/android_openssl/android_openssl.cmake)
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${QT_RESOURCES})
if (NOT DEFINED CACHE{OSM_API_KEY})
    message(FATAL_ERROR "OSM_API_KEY has to be set as a -D option!")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE API_KEY="${OSM_API_KEY}") # pass API key via -D in cmake
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE MAIN_QML="${CMAKE_CURRENT_LIST_DIR}/qml/Main.qml")
endif()

if (APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/resources/mac/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_ICON_FILE "PastViewer"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist
    )
    set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
elseif(ANDROID)
    add_android_openssl_libraries(${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/android"
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
    )
endif()

qt6_import_qml_plugins(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Quick
    Qt6::QuickLayouts
    Qt6::QuickControls2
    Qt6::Location
    Qt6::Positioning
    Qt6::PositioningQuick
    glog::glog
)

file(GLOB_RECURSE ABS_QML CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/qml/*.qml"
)

AbsToRelPath(REL_QML "${CMAKE_CURRENT_SOURCE_DIR}" ${ABS_QML})

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    RESOURCES
        "src/App/qml/Helpers/colors.js"
    QML_FILES
        ${REL_QML}
)
