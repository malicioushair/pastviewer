cmake_minimum_required(VERSION 3.28)
project(PastViewer VERSION 1.3.1 LANGUAGES CXX)
set(APPSTORE_BUILD_NUMBER "${PROJECT_VERSION}" CACHE STRING "AppStore bundle build number")
set(PLAYSTORE_BUILD_NUMBER "${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}" CACHE STRING "Google Play versionCode")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)

option(ENABLE_ASAN "Enable AddressSanitizer for memory error diagnosis" OFF)
if(ENABLE_ASAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "ENABLE_ASAN requires a Clang or GCC compiler")
    endif()
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources/android")

include_directories(src)

include(cmake/Helpers.cmake)

include(src/App/App.cmake)

if (NOT ANDROID AND NOT IOS)
    option(BUILD_TESTS "Build unit tests" ON)
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)


install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION  .
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .)
