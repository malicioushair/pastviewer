cmake_minimum_required(VERSION 3.28)
project(PastViewer VERSION 0.3.0 LANGUAGES CXX)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/android")

include_directories(src)

include(cmake/Helpers.cmake)

include(src/App/App.cmake)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)


install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION  .
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .)
