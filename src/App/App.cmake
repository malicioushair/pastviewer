include(cmake/Helpers.cmake)

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
    # Use different Info.plist template for iOS vs macOS
    set(OS_SUB_DIR mac)
    if(IOS)
        set(OS_SUB_DIR ios)
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/resources/${OS_SUB_DIR}/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist
    )

    if(IOS)
        set(IOS_ASSETS_DIR "${CMAKE_SOURCE_DIR}/resources/${OS_SUB_DIR}/Assets.xcassets")

        if(EXISTS ${IOS_ASSETS_DIR})
            # Detect platform (simulator vs device)
            if(CMAKE_OSX_SYSROOT MATCHES "simulator")
                set(ACTOOL_PLATFORM "iphonesimulator")
            else()
                set(ACTOOL_PLATFORM "iphoneos")
            endif()

            set(ASSETS_CAR_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Assets.car")

            file(GLOB_RECURSE ASSET_FILES "${IOS_ASSETS_DIR}/*")

            set(ICON_60_2X "${CMAKE_CURRENT_BINARY_DIR}/AppIcon60x60@2x.png")
            set(ICON_76_2X "${CMAKE_CURRENT_BINARY_DIR}/AppIcon76x76@2x~ipad.png")

            # Use actool to compile the asset catalog
            add_custom_command(
                OUTPUT ${ASSETS_CAR_OUTPUT} ${ICON_60_2X} ${ICON_76_2X}
                COMMAND xcrun actool
                    --compile ${CMAKE_CURRENT_BINARY_DIR}
                    --platform ${ACTOOL_PLATFORM}
                    --minimum-deployment-target 13.0
                    --app-icon AppIcon
                    --output-partial-info-plist ${CMAKE_CURRENT_BINARY_DIR}/AssetCatalog-Info.plist
                    ${IOS_ASSETS_DIR}
                DEPENDS ${ASSET_FILES}
                COMMENT "Compiling asset catalog for ${ACTOOL_PLATFORM}"
                VERBATIM
            )

            # Add the compiled assets as a custom target
            add_custom_target(CompileAssets ALL DEPENDS ${ASSETS_CAR_OUTPUT} ${ICON_60_2X} ${ICON_76_2X})
            add_dependencies(${PROJECT_NAME} CompileAssets)

            # Add compiled asset catalog and icon files to the bundle
            set_source_files_properties(${ASSETS_CAR_OUTPUT} ${ICON_60_2X} ${ICON_76_2X} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
                GENERATED TRUE
            )
            target_sources(${PROJECT_NAME} PRIVATE ${ASSETS_CAR_OUTPUT} ${ICON_60_2X} ${ICON_76_2X})
        endif()
    endif()
elseif(ANDROID)
    add_android_openssl_libraries(${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/android"
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
        QT_ANDROID_VERSION_CODE "${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}"
        QT_ANDROID_VERSION_NAME "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
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
