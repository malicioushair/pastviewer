cmake_minimum_required(VERSION 3.28)
project(PastViewer VERSION 0.1.0 LANGUAGES CXX)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")

include_directories(src)

include(cmake/Helpers.cmake)

# execute_process(
#   COMMAND conan install ${CMAKE_SOURCE_DIR}
#           --build=missing
#           --output-folder=${CMAKE_BINARY_DIR}
#           -s build_type=${CMAKE_BUILD_TYPE}
# )

include(src/TorrentDownloader/TorrentDownloader.cmake)
include(src/App/App.cmake)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# qt_generate_deploy_qml_app_script(
    # TARGET ${PROJECT_NAME}
    # OUTPUT_SCRIPT deploy_script
# )

install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION  .
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .)

# install(SCRIPT ${deploy_script})

if (ON_BUILD_SERVER)
    message("ON_BUILD_SERVER!")
    SignAndDeploy()
endif()