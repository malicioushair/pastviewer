cmake_minimum_required(VERSION 3.28)
project(ToolingFixture LANGUAGES NONE)
include("${TOOLING_HELPER}")

# Stand-ins for Qt's metadata-bearing targets let this test exercise the build
# graph without compiling or linking an application. The real Qt registrar runs.
add_executable(Qt6::qmltyperegistrar IMPORTED GLOBAL)
set_property(TARGET Qt6::qmltyperegistrar PROPERTY IMPORTED_LOCATION "${REGISTRAR}")
foreach(_target ToolingFixture OtherBackingTarget)
    add_custom_target(${_target})
    set_property(TARGET ${_target} PROPERTY INTERFACE_QT_META_TYPES_BUILD_FILE "${METATYPES_INPUT}")
endforeach()
set_property(TARGET ToolingFixture PROPERTY QML_SINGLETON_TOOLING_DEPENDENCIES
    "QtQml 2.0" "QtQml.Models 2.0")

add_qml_singleton_tooling(TourController)
add_qml_singleton_tooling(Test::OtherController
    TARGET OtherBackingTarget MODULE Example.Services QML_NAME Services VERSION 2.3
    DEPENDENCIES "QtQml 2.0")
if(TEST_DUPLICATE)
    add_qml_singleton_tooling(TourController)
endif()
