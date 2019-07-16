# - Basic CMake setup
# Check version meets ou requirements
# Declare project, which will configure compiler for us
cmake_minimum_required(VERSION 3.3)
project(FalaiseMockTrackerClusterizerPlugin VERSION "2.1.0")

# Modules use Falaise, so we need to locate this or fail
# find_package(Falaise REQUIRED)

# # Build a dynamic library from our sources
add_library(Falaise_MockTrackerClusterizer SHARED
  MockTrackerClusterizer/mock_tracker_clustering_driver.cc
  MockTrackerClusterizer/mock_tracker_clustering_driver.h
  MockTrackerClusterizer/mock_tracker_clustering_module.cc
  MockTrackerClusterizer/mock_tracker_clustering_module.h
  )
target_include_directories(Falaise_MockTrackerClusterizer PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  )
target_link_libraries(Falaise_MockTrackerClusterizer PRIVATE Falaise)

# Apple linker requires dynamic lookup of symbols, so we
# add link flags on this platform
if(APPLE)
  set_target_properties(Falaise_MockTrackerClusterizer
    PROPERTIES LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

# Install it:
install(TARGETS Falaise_MockTrackerClusterizer DESTINATION ${CMAKE_INSTALL_LIBDIR}/Falaise/modules)

# Test support:
option(FalaiseMockTrackerClusterizerPlugin_ENABLE_TESTING "Build unit testing system for FalaiseMockTrackerClusterizerPlugin" ON)
if(FalaiseMockTrackerClusterizerPlugin_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
