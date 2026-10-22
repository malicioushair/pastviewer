cmake_minimum_required(VERSION 3.28)

# Find required packages
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Location Network Positioning REQUIRED)

# Enable testing
enable_testing()

# Create test executable
add_executable(PastViewerTests
    DirectionUtilsTest.cpp
    FileHashTest.cpp
    PhotoNotificationCoordinatorTest.cpp
    PhotoNotificationPresenterTest.cpp
    PhotoNotificationSettingsTest.cpp
    PhotoProximityTrackerTest.cpp
    UniqueCircularBufferTest.cpp
    ClusterModelTest.cpp
    PlacedTourPathTest.cpp
    PlacedTourStopTest.cpp
    StopAudioTest.cpp
    TourPurchaseOwnershipTest.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/GuiController/platform/mac/Logic.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/PurchaseController/platform/mac/Logic.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/PurchaseController/PurchaseController.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/TourController/TourController.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/ModelController/PositionSourceAdapter.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/AvailableToursModel/AvailableToursModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/TourDraftsModel/TourDraftsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/TourStopsModel/TourStopsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/ClusterModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/ScreenObjectsModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Models/BaseModel.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/ModelController/PhotoProximityTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/ModelController/PhotoNotificationCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/GuiController/PhotoNotificationPresenter.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/GuiController/TipPromptTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/App/Tours/DisplayTours.h
    ${CMAKE_SOURCE_DIR}/src/App/Utils/Range.h
)

# Include directories
target_include_directories(PastViewerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/App/Controllers/ModelController
)

# Link against gtest and the required libraries
target_compile_definitions(PastViewerTests PRIVATE "ADMIN_TOKEN=\"test-token\"")

target_link_libraries(PastViewerTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    Qt6::Gui
    Qt6::Location
    Qt6::Network
    Qt6::Positioning
    glog::glog
)

# Set C++ standard
set_target_properties(PastViewerTests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
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

# Tooling metadata must follow the C++ API without changing runtime registration.
add_test(NAME ImperativeQmlTypes
    COMMAND ${CMAKE_COMMAND}
        "-DMOC=$<TARGET_FILE:Qt6::moc>"
        "-DREGISTRAR=$<TARGET_FILE:Qt6::qmltyperegistrar>"
        "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/imperative_qmltypes"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestImperativeQmlTypes.cmake"
)
