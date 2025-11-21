include(cmake/Helpers.cmake)

find_package(glog REQUIRED)
find_package(gflags CONFIG REQUIRED)

# sentry-native is only used on macOS. Android uses Sentry Android SDK (via Gradle).
if(NOT ANDROID)
    # Ensure sentry_DIR is set if not already provided
    if(NOT sentry_DIR AND EXISTS "${CMAKE_BINARY_DIR}/sentry-config.cmake")
        set(sentry_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Path to sentry config")
    endif()
    find_package(sentry CONFIG REQUIRED)
endif()

find_package(Qt6 COMPONENTS Core Gui Quick QuickLayouts QuickControls2 Location Positioning PositioningQuick  REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

include_sources(SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)
include(ext/android_openssl/android_openssl.cmake)
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${QT_RESOURCES})

# Add Android backtrace stub to prevent UnsatisfiedLinkError
# glog uses backtrace() which is not available on Android
if(ANDROID)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/android_backtrace_stub.cpp")
endif()
if (NOT DEFINED CACHE{OSM_API_KEY})
    message(FATAL_ERROR "OSM_API_KEY has to be set as a -D option!")
endif()
if (NOT DEFINED CACHE{SENTRY_DSN})
    message(FATAL_ERROR "SENTRY_DSN has to be set as a -D option!")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE API_KEY="${OSM_API_KEY}") # pass API key via -D in cmake
target_compile_definitions(${PROJECT_NAME} PRIVATE SENTRY_DSN="${SENTRY_DSN}") # pass SENTRY DSN key via -D in cmake
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_MAJOR="${CMAKE_PROJECT_VERSION_MAJOR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_MINOR="${CMAKE_PROJECT_VERSION_MINOR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_PATCH="${CMAKE_PROJECT_VERSION_PATCH}")
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
        QT_ANDROID_VERSION_CODE "${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}"
        QT_ANDROID_VERSION_NAME "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
    )

    add_sentry_android_dependency(${PROJECT_NAME})
    
    # Link against libc++_shared to provide backtrace symbol for glog
    # This is needed because glog uses backtrace for stack traces on Android
    find_library(ANDROID_CPP_SHARED_LIB c++_shared PATHS ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI} NO_DEFAULT_PATH)
    if(ANDROID_CPP_SHARED_LIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ANDROID_CPP_SHARED_LIB})
        message(STATUS "Found libc++_shared: ${ANDROID_CPP_SHARED_LIB}")
    else()
        # Fallback: try to find it in the standard Android NDK location
        find_library(ANDROID_CPP_SHARED_LIB c++_shared)
        if(ANDROID_CPP_SHARED_LIB)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${ANDROID_CPP_SHARED_LIB})
            message(STATUS "Found libc++_shared (fallback): ${ANDROID_CPP_SHARED_LIB}")
        else()
            message(WARNING "Could not find libc++_shared. glog may fail to load due to missing backtrace symbol.")
        endif()
    endif()
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

# sentry-native is only linked on macOS. Android uses Sentry Android SDK.
if(NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE sentry::sentry)
endif()

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
        "src/App/qml/Helpers/utils.js"
    QML_FILES
        ${REL_QML}
)
