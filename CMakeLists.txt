cmake_minimum_required(VERSION 3.15)
project(AutoThematicThesaurus VERSION 1.0)


# Compiler cache configuration for incremental builds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set(CMAKE_VERBOSE_MAKEFILE OFF)
endif()
message(STATUS "Compiler cache: ${CCACHE_PROGRAM}")


# C++ standard and compiler configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Propagate C++ standard to Abseil" FORCE)


# Optimization configuration
set(COMMON_OPTIMIZATION_FLAGS "-ffast-math -march=native -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 ${COMMON_OPTIMIZATION_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_OPTIMIZATION_FLAGS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "Suppress developer warnings")


# CMake policy configuration for compatibility
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()


# FetchContent configuration to prevent conflicts with cached builds
set(FETCHCONTENT_QUIET ON)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)


# TensorFlow Lite logging suppression
# Prevents linking issues with Abseil logging by disabling TensorFlow Lite diagnostics
add_definitions(-DTENSORFLOW_LITE_DISABLE_LOG=1)
add_definitions(-DTF_LITE_DISABLE_XNNPACK_DELEGATE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DABSL_MIN_LOG_LEVEL=3")


# XMorphy submodule configuration
set(XMORPHY_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/XMorphy")
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "XMorphy submodule not found at ${XMORPHY_PROJECT_PATH}")
endif()


# Verify XMorphy dependency submodules
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/contrib/tensorflow")
    message(FATAL_ERROR "XMorphy contrib/tensorflow not found. Execute: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/contrib/icu")
    message(FATAL_ERROR "XMorphy contrib/icu not found. Execute: git submodule update --init --recursive")
endif()


# Add XMorphy as submodule
add_subdirectory("${XMORPHY_PROJECT_PATH}")
message(STATUS "XMorphy submodule loaded from: ${XMORPHY_PROJECT_PATH}")


# Project source directory configuration
include_directories(
    src/grammar_component
    src/grammar_patterns
    src/phrases_collecting
    src/text_corpus
    src/utils
)


# Collect project source files
file(GLOB_RECURSE SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/*/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*/*.h
)


# Boost library configuration
find_package(Boost REQUIRED COMPONENTS filesystem program_options)
if(Boost_FOUND)
    message(STATUS "Boost library located: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Boost library not found")
endif()


# SQLite3 database library
find_package(SQLite3 REQUIRED)


# Static library build from project sources
set(MAIN_CPP "${PROJECT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM SOURCE_FILES "${MAIN_CPP}")
add_library(AutoThematicLib STATIC ${SOURCE_FILES})
target_include_directories(AutoThematicLib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(AutoThematicLib PRIVATE -Wno-unused)


# Main executable configuration
add_executable(AutoThematicThesaurus ${MAIN_CPP})
target_link_libraries(AutoThematicThesaurus PRIVATE AutoThematicLib Boost::filesystem Boost::program_options)
target_include_directories(AutoThematicThesaurus PRIVATE ${PROJECT_SOURCE_DIR}/src)


# Build type configuration
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Specifies build configuration type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()


# Installation directory configuration
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Installation directory" FORCE)


# Position-independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# XMorphy library linkage
target_link_libraries(AutoThematicLib PUBLIC Xmorphy)
target_link_libraries(AutoThematicLib PUBLIC ${SQLite3_LIBRARIES})


# Copy XMorphy data files to build directory
file(COPY ${XMORPHY_PROJECT_PATH}/data/dicts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${XMORPHY_PROJECT_PATH}/data/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# Project resources configuration
include_directories(${PROJECT_SOURCE_DIR}/my_data)
include_directories(${PROJECT_SOURCE_DIR}/x_morphy_resuls)


# Google Test framework integration
find_package(GTest REQUIRED)
message(STATUS "Google Test framework located: ${GTest_INCLUDE_DIRS}")
enable_testing()
add_subdirectory(tests)


# Build configuration summary
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "XMorphy Path: ${XMORPHY_PROJECT_PATH}")
message(STATUS "Boost Path: ${Boost_INCLUDE_DIRS}")
message(STATUS "SQLite3 Path: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "===========================")
message(STATUS "")