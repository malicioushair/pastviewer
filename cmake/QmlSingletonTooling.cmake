include_guard(GLOBAL)

# Tooling for qmlRegisterSingletonInstance; does not change runtime registration.
# Call after qt_add_qml_module in the same CMake directory as the backing target.
#
# add_qml_singleton_tooling(MyController)
# add_qml_singleton_tooling(App::Controller
#     TARGET MyApp MODULE App.Services QML_NAME Services VERSION 2.3
#     DEPENDENCIES "QtQml 2.0")
#
# Defaults: TARGET=${PROJECT_NAME}, QML_NAME=unqualified class name,
# MODULE=QML_NAME, VERSION=1.0. Each invocation describes a separate module URI.
# Shared dependencies can be set once on the backing target using the
# QML_SINGLETON_TOOLING_DEPENDENCIES property; otherwise QtQml 2.0 is used.
function(add_qml_singleton_tooling class_name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "TARGET;MODULE;QML_NAME;VERSION" "DEPENDENCIES")
    if(NOT class_name OR arg_UNPARSED_ARGUMENTS OR arg_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "Invalid arguments to add_qml_singleton_tooling(${class_name})")
    endif()
    if(NOT arg_TARGET)
        set(arg_TARGET "${PROJECT_NAME}")
    endif()
    if(NOT TARGET "${arg_TARGET}")
        message(FATAL_ERROR "QML tooling backing target does not exist: ${arg_TARGET}")
    endif()
    if(NOT arg_QML_NAME)
        string(REGEX REPLACE "^.*::" "" arg_QML_NAME "${class_name}")
    endif()
    if(NOT arg_MODULE)
        set(arg_MODULE "${arg_QML_NAME}")
    endif()
    if(NOT arg_VERSION)
        set(arg_VERSION "1.0")
    endif()
    if(NOT arg_QML_NAME MATCHES "^[A-Z][A-Za-z0-9_]*$" OR
       NOT arg_MODULE MATCHES "^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$" OR
       NOT arg_VERSION MATCHES "^[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR "Invalid QML name, module URI or major.minor version for ${class_name}")
    endif()
    if(NOT DEFINED arg_DEPENDENCIES)
        get_property(arg_DEPENDENCIES TARGET "${arg_TARGET}"
            PROPERTY QML_SINGLETON_TOOLING_DEPENDENCIES)
        if(NOT arg_DEPENDENCIES)
            set(arg_DEPENDENCIES "QtQml 2.0")
        endif()
    endif()
    get_target_property(_metatypes "${arg_TARGET}" INTERFACE_QT_META_TYPES_BUILD_FILE)
    if(NOT _metatypes OR NOT TARGET Qt6::qmltyperegistrar)
        message(FATAL_ERROR "Call qt_add_qml_module(${arg_TARGET} ...) before registering singleton tooling")
    endif()
    get_target_property(_binary_dir "${arg_TARGET}" BINARY_DIR)
    string(REPLACE "." "/" _module_path "${arg_MODULE}")
    set(_output_dir "${_binary_dir}/${_module_path}")
    # Reject two producers of the same qmldir, including the backing QML module.
    get_target_property(_backing_uri "${arg_TARGET}" QT_QML_MODULE_URI)
    string(SHA256 _module_key "${_output_dir}")
    get_property(_existing GLOBAL PROPERTY "QML_SINGLETON_TOOLING_${_module_key}")
    if(_existing OR arg_MODULE STREQUAL _backing_uri)
        message(FATAL_ERROR "QML module ${arg_MODULE} already has a tooling producer")
    endif()
    set_property(GLOBAL PROPERTY "QML_SINGLETON_TOOLING_${_module_key}" "${class_name}")
    set(_qmltypes "${_output_dir}/${arg_QML_NAME}.qmltypes")
    set(_qmldir "module ${arg_MODULE}\ntypeinfo ${arg_QML_NAME}.qmltypes\n")
    foreach(_dependency IN LISTS arg_DEPENDENCIES)
        string(APPEND _qmldir "depends ${_dependency}\n")
    endforeach()
    file(GENERATE OUTPUT "${_output_dir}/qmldir" CONTENT "${_qmldir}")

    # qt_add_qml_module exposes linked modules' metatypes through target sources.
    set(_foreign_types "$<REMOVE_DUPLICATES:$<FILTER:$<TARGET_PROPERTY:${arg_TARGET},SOURCES>,INCLUDE,metatypes.json$>>")
    string(SUBSTRING "${_module_key}" 0 12 _suffix)
    set(_tooling_target "${arg_TARGET}_${arg_QML_NAME}_${_suffix}_qmltypes")
    set(_foreign_file "${_binary_dir}/.qt/${_tooling_target}_$<CONFIG>.txt")
    file(GENERATE OUTPUT "${_foreign_file}"
        CONTENT "$<IF:$<BOOL:${_foreign_types}>,--foreign-types=$<JOIN:${_foreign_types},$<COMMA>>,\n>")
    set(_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GenerateImperativeQmlTypes.cmake")
    add_custom_command(
        OUTPUT "${_qmltypes}"
        BYPRODUCTS "${_qmltypes}.metatypes.json"
        COMMAND ${CMAKE_COMMAND}
            "-DINPUT=${_metatypes}" "-DOUTPUT=${_qmltypes}"
            "-DCLASS_NAME=${class_name}" "-DQML_NAME=${arg_QML_NAME}"
            "-DMODULE_NAME=${arg_MODULE}" "-DMODULE_VERSION=${arg_VERSION}"
            "-DREGISTRAR=$<TARGET_FILE:Qt6::qmltyperegistrar>"
            "-DFOREIGN_TYPES_FILE=${_foreign_file}" -P "${_script}"
        DEPENDS "${_metatypes}" "${_script}" "${_foreign_file}"
            Qt6::qmltyperegistrar ${_foreign_types}
        COMMENT "Generating ${arg_MODULE} tooling metadata from ${class_name}"
        VERBATIM
    )
    add_custom_target("${_tooling_target}" DEPENDS "${_qmltypes}")
    # A source dependency avoids feeding this target back into AUTOMOC.
    set_source_files_properties("${_qmltypes}" PROPERTIES HEADER_FILE_ONLY TRUE SKIP_AUTOGEN TRUE)
    target_sources("${arg_TARGET}" PRIVATE "${_qmltypes}")
    add_dependencies("${_tooling_target}" "${arg_TARGET}")

    if(NOT TARGET all_qmltyperegistrations)
        add_custom_target(all_qmltyperegistrations)
    endif()
    add_dependencies(all_qmltyperegistrations "${_tooling_target}")
endfunction()
