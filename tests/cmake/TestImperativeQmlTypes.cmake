# Run with -DMOC=... -DREGISTRAR=... -DTEST_DIR=... -P <this file>.
foreach(_required MOC REGISTRAR TEST_DIR)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "Missing ${_required}")
    endif()
endforeach()
get_filename_component(_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_generator "${_root}/cmake/GenerateImperativeQmlTypes.cmake")
file(MAKE_DIRECTORY "${TEST_DIR}")
set(_header "${TEST_DIR}/Controller.h")
set(_moc "${TEST_DIR}/moc_Controller.cpp")
set(_input "${TEST_DIR}/metatypes.json")
set(_output "${TEST_DIR}/TourController.qmltypes")

function(generate_fixture extra_api)
    file(WRITE "${_header}" "class TourController : public QObject {\nQ_OBJECT\nQ_PROPERTY(QString title READ title NOTIFY titleChanged)\npublic:\nQString title() const;\nQ_INVOKABLE void start();\n${extra_api}\nsignals:\nvoid titleChanged();\n};\nnamespace Test { class OtherController : public QObject {\nQ_OBJECT\npublic:\nQ_INVOKABLE int otherMethod() const;\n}; }\n")
    execute_process(COMMAND "${MOC}" --output-json -o "${_moc}" "${_header}"
        RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Fixture moc failed")
    endif()
    execute_process(COMMAND "${MOC}" --collect-json -o "${_input}" "${_moc}.json"
        RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Fixture metadata collection failed")
    endif()
    file(SHA256 "${_input}" _before)
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DINPUT=${_input}" "-DOUTPUT=${_output}"
        -DCLASS_NAME=TourController -DQML_NAME=TourController -DMODULE_NAME=TourController
        "-DREGISTRAR=${REGISTRAR}" -P "${_generator}"
        RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Generation failed: ${_stdout}${_stderr}")
    endif()
    file(SHA256 "${_input}" _after)
    if(NOT _before STREQUAL _after OR EXISTS "${_output}.registration.cpp")
        message(FATAL_ERROR "Generation modified input or retained runtime registration code")
    endif()
endfunction()

function(expect text)
    file(READ "${_output}" _generated)
    string(FIND "${_generated}" "${text}" _position)
    if(_position EQUAL -1)
        message(FATAL_ERROR "Generated metadata missing: ${text}")
    endif()
endfunction()

generate_fixture("")
expect("isSingleton: true")
expect("TourController/TourController 1.0")
expect("name: \"title\"")
expect("name: \"titleChanged\"")
expect("name: \"start\"")
generate_fixture("Q_INVOKABLE QString addedMethod(int value) const;\nQ_PROPERTY(int addedProperty READ addedProperty)\nint addedProperty() const;")
expect("name: \"addedMethod\"")
expect("name: \"addedProperty\"")
expect("name: \"value\"; type: \"int\"")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DINPUT=${_input}" "-DOUTPUT=${_output}"
    -DCLASS_NAME=MissingController -DQML_NAME=TourController -DMODULE_NAME=TourController
    "-DREGISTRAR=${REGISTRAR}" -P "${_generator}"
    RESULT_VARIABLE _result OUTPUT_QUIET ERROR_QUIET)
if(_result EQUAL 0)
    message(FATAL_ERROR "Missing controller should fail generation")
endif()
message(STATUS "Imperative QML metadata: API updates, singleton export, input preservation and missing-class checks passed")

# Exercise the public one-line function twice in one project, including a
# namespaced class, a different target, a dotted URI and overridden QML version.
set(_fixture_source "${CMAKE_CURRENT_LIST_DIR}/SingletonToolingFixture")
set(_fixture_build "${TEST_DIR}/function-fixture")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_fixture_source}" -B "${_fixture_build}"
    "-DTOOLING_HELPER=${_root}/cmake/QmlSingletonTooling.cmake"
    "-DMETATYPES_INPUT=${_input}" "-DREGISTRAR=${REGISTRAR}" -DTEST_DUPLICATE=OFF
    RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Helper fixture configuration failed: ${_stdout}${_stderr}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_fixture_build}"
    --target all_qmltyperegistrations
    RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Helper fixture build failed: ${_stdout}${_stderr}")
endif()
set(_output "${_fixture_build}/TourController/TourController.qmltypes")
expect("TourController/TourController 1.0")
expect("name: \"addedMethod\"")
set(_output "${_fixture_build}/Example/Services/Services.qmltypes")
expect("name: \"Test::OtherController\"")
expect("Example.Services/Services 2.0")
expect("name: \"otherMethod\"")
file(READ "${_fixture_build}/TourController/qmldir" _default_qmldir)
file(READ "${_fixture_build}/Example/Services/qmldir" _custom_qmldir)
if(NOT _default_qmldir MATCHES "depends QtQml.Models 2.0" OR
   NOT _custom_qmldir MATCHES "module Example.Services" OR
   _custom_qmldir MATCHES "QtQml.Models")
    message(FATAL_ERROR "Module URI or dependency defaults/override were not applied")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_fixture_source}" -B "${_fixture_build}"
    -DTEST_DUPLICATE=ON RESULT_VARIABLE _result OUTPUT_QUIET ERROR_VARIABLE _stderr)
if(_result EQUAL 0 OR NOT _stderr MATCHES "already has a tooling producer")
    message(FATAL_ERROR "Duplicate module producers must fail explicitly: ${_stderr}")
endif()
message(STATUS "Singleton tooling helper: two modules, namespace, target/name/version overrides and duplicate detection passed")
