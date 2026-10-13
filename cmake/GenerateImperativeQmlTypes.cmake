# Generate tooling metadata only for an imperatively registered QObject.
# Never compile the registrar's C++ output: runtime registration stays in the app.
foreach(_required INPUT OUTPUT CLASS_NAME QML_NAME MODULE_NAME REGISTRAR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${_required} for imperative QML tooling generation")
    endif()
endforeach()

if(NOT DEFINED MODULE_VERSION)
    set(MODULE_VERSION "1.0")
endif()
if(NOT MODULE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR "Invalid module version: ${MODULE_VERSION}")
endif()
set(_major_version "${CMAKE_MATCH_1}")
set(_minor_version "${CMAKE_MATCH_2}")

file(READ "${INPUT}" _metadata)
string(JSON _unit_count LENGTH "${_metadata}")
set(_matches 0)
set(_units "[]")
if(_unit_count GREATER 0)
    math(EXPR _last_unit "${_unit_count} - 1")
    foreach(_unit_index RANGE 0 ${_last_unit})
        string(JSON _unit GET "${_metadata}" ${_unit_index})
        string(JSON _class_count LENGTH "${_unit}" classes)
        if(_class_count EQUAL 0)
            continue()
        endif()
        math(EXPR _last_class "${_class_count} - 1")
        foreach(_class_index RANGE 0 ${_last_class})
            string(JSON _class GET "${_unit}" classes ${_class_index})
            string(JSON _name GET "${_class}" qualifiedClassName)
            if(NOT _name STREQUAL CLASS_NAME)
                continue()
            endif()
            math(EXPR _matches "${_matches} + 1")
            # Only annotate a copy consumed by qmltyperegistrar. The application's
            # moc metadata and its other QML module exports remain untouched.
            string(JSON _infos ERROR_VARIABLE _missing GET "${_class}" classInfos)
            if(_missing)
                set(_infos "[]")
            endif()
            string(JSON _info_count LENGTH "${_infos}")
            string(JSON _infos SET "${_infos}" ${_info_count}
                "{\"name\":\"QML.Element\",\"value\":\"${QML_NAME}\"}")
            math(EXPR _info_count "${_info_count} + 1")
            string(JSON _infos SET "${_infos}" ${_info_count}
                "{\"name\":\"QML.Singleton\",\"value\":\"true\"}")
            string(JSON _class SET "${_class}" classInfos "${_infos}")
            string(JSON _selected_unit SET "${_unit}" classes "[${_class}]")
            string(JSON _units SET "${_units}" 0 "${_selected_unit}")
        endforeach()
    endforeach()
endif()
if(NOT _matches EQUAL 1)
    message(FATAL_ERROR "Expected exactly one ${CLASS_NAME} in ${INPUT}, found ${_matches}")
endif()

get_filename_component(_output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_output_dir}")
set(_annotated "${OUTPUT}.metatypes.json")
set(_unused_cpp "${OUTPUT}.registration.cpp")
file(WRITE "${_annotated}" "${_units}\n")
set(_foreign_args)
if(DEFINED FOREIGN_TYPES_FILE)
    list(APPEND _foreign_args "@${FOREIGN_TYPES_FILE}")
endif()
execute_process(
    COMMAND "${REGISTRAR}" ${_foreign_args}
        "--import-name=${MODULE_NAME}" "--major-version=${_major_version}" "--minor-version=${_minor_version}"
        "--generate-qmltypes=${OUTPUT}" -o "${_unused_cpp}" "${_annotated}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
)
# This output exists only because qmltyperegistrar also emits C++ by default.
file(REMOVE "${_unused_cpp}")
if(NOT _result STREQUAL "0")
    file(REMOVE "${OUTPUT}")
    message(FATAL_ERROR "qmltyperegistrar failed (${_result}):\n${_stdout}${_stderr}")
endif()
if(NOT _stderr STREQUAL "")
    message(WARNING "qmltyperegistrar: ${_stderr}")
endif()
