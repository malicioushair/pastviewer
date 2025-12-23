cmake_minimum_required(VERSION 3.28)

# Find required packages
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)

# Enable testing
enable_testing()

# Create test executable
add_executable(PastViewerTests
    DirectionUtilsTest.cpp
    UniqueCircularBufferTest.cpp
)

# Include directories
target_include_directories(PastViewerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link against gtest and the required libraries
target_link_libraries(PastViewerTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    glog::glog
)

# Set C++ standard
set_target_properties(PastViewerTests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(PastViewerTests)

# Add a custom target to run tests
add_custom_target(run_tests
    COMMAND PastViewerTests
    DEPENDS PastViewerTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
)

