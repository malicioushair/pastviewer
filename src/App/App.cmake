include(cmake/Helpers.cmake)

if(IOS)
    list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_BINARY_DIR}")
endif()

find_package(glog REQUIRED)
find_package(gflags CONFIG REQUIRED)

# sentry-native on macOS and iOS. Android uses Sentry Android SDK (via Gradle).
if(NOT ANDROID)
    if(NOT sentry_DIR AND EXISTS "${CMAKE_BINARY_DIR}/sentry-config.cmake")
        set(sentry_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Path to sentry config")
    endif()
    if(IOS AND NOT EXISTS "${CMAKE_BINARY_DIR}/sentry-config.cmake")
        message(FATAL_ERROR
            "sentry-config.cmake not found in ${CMAKE_BINARY_DIR}. "
            "Install Conan deps here (same folder as glog-config.cmake), e.g. "
            "conan install <source-dir> --output-folder=. --build=missing with your iOS profile, "
            "then configure CMake again.")
    endif()
    find_package(sentry CONFIG REQUIRED)
endif()

find_package(Qt6 COMPONENTS
    Core
    Gui
    Quick
    QuickLayouts
    QuickControls2
    Location
    Positioning
    PositioningQuick
    Multimedia
    LinguistTools
REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

include_sources(SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.mm"
)
include(ext/android_openssl/android_openssl.cmake)
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${QT_RESOURCES})

if(IOS)
    # https://doc.qt.io/qt-6/ios-platform-notes.html — absolute path required
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_IOS_LAUNCH_SCREEN "${CMAKE_SOURCE_DIR}/resources/ios/LaunchScreen.storyboard"
    )
    # Symlinks are unreliable with actool; keep a real PNG (synced from Android foreground).
    configure_file(
        "${CMAKE_SOURCE_DIR}/resources/android/res/mipmap-hdpi/ic_launcher_foreground.png"
        "${CMAKE_SOURCE_DIR}/resources/ios/Assets.xcassets/AppIcon.appiconset/AppIcon1024x1024.png"
        COPYONLY
    )
    set(_pastviewer_ios_assets "${CMAKE_SOURCE_DIR}/resources/ios/Assets.xcassets")
    target_sources(${PROJECT_NAME} PRIVATE "${_pastviewer_ios_assets}")
    # Mark as asset catalog so Xcode compiles it (actool), not a plain resource copy.
    set_source_files_properties("${_pastviewer_ios_assets}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )
    set(_pastviewer_ios_cacert "${CMAKE_SOURCE_DIR}/resources/ios/certs/curl/cacert.pem")
    if(NOT EXISTS "${_pastviewer_ios_cacert}")
        message(FATAL_ERROR "iOS CA bundle not found at ${_pastviewer_ios_cacert}")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${_pastviewer_ios_cacert}")
    set_source_files_properties("${_pastviewer_ios_cacert}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/certs/curl"
    )
endif()

# Add Android backtrace stub to prevent UnsatisfiedLinkError
# glog uses backtrace() which is not available on Android
if(ANDROID)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/android_backtrace_stub.cpp")
endif()
if (NOT DEFINED CACHE{OSM_API_KEY} OR "${OSM_API_KEY}" STREQUAL "")
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
    if(IOS)
        set(_pastviewer_plist "${CMAKE_SOURCE_DIR}/resources/ios/Info.plist.in")
        configure_file(${CMAKE_SOURCE_DIR}/resources/ios/ExportOptions-app-store.plist.in ${CMAKE_BINARY_DIR}/ExportOptions-app-store.plist @ONLY)
    else()
        configure_file(${CMAKE_SOURCE_DIR}/resources/mac/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
        set(_pastviewer_plist "${CMAKE_BINARY_DIR}/Info.plist")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${_pastviewer_plist}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APPLE_APP_REVERSED_DOMAIN}"
    )
    if(NOT IOS)
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "PastViewer")
    endif()
    if(IOS)
        if("${APPLE_TEAM_ID}" STREQUAL "")
            message(FATAL_ERROR "APPLE_TEAM_ID required. Pass it via -D cmake option.")
        endif()

        set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_TEAM_ID}"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
            XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${APPLE_PROVISION_PROFILE_NAME}"
        )
    endif()
    if(DEFINED APP_ICON)
        set_source_files_properties(${APP_ICON} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
    endif()
elseif(ANDROID)
    add_android_openssl_libraries(${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/android"
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
        QT_ANDROID_VERSION_CODE "${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}"
        QT_ANDROID_VERSION_NAME "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
    )

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
    Qt6::Multimedia
    glog::glog
)

# sentry-native on macOS and iOS. Android uses Sentry Android SDK.
if(NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE sentry::sentry)
endif()

# Qt 6 static FFmpeg media plugin does not pull in libav*; link Qt's bundled xcframeworks.
if(IOS)
    enable_language(OBJCXX)
    qt_add_ios_ffmpeg_libraries(${PROJECT_NAME})
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
        "src/App/qml/resources/share.png"
    QML_FILES
        ${REL_QML}
)

qt_add_translations(${PROJECT_NAME}
    TS_FILES
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_de.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_en.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_es.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_fr.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_it.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_ja.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_ko.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_pt.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_ru.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_sr.ts
        ${CMAKE_CURRENT_LIST_DIR}/translations/PastViewer_zh_CN.ts
    RESOURCE_PREFIX
        /i18n
    LUPDATE_OPTIONS
        -no-obsolete
    LRELEASE_OPTIONS
        -compress
        -nounfinished
        -removeidentical
)
