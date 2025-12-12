cmake_minimum_required(VERSION 3.15)
project(AutoThematicThesaurus VERSION 1.0)

# Enable ccache for faster incremental builds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set(CMAKE_VERBOSE_MAKEFILE OFF)
endif()
message(STATUS "Using ccache: ${CCACHE_PROGRAM}")

# Set C++ standard and compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Enable C++ standard propagation to Abseil" FORCE)

# Optimization flags
set(COMMON_OPTIMIZATION_FLAGS "-ffast-math -march=native -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${COMMON_OPTIMIZATION_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_OPTIMIZATION_FLAGS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")

# Ensure compatibility with modern CMake policies
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

# ===== PREVENT FETCHCONTENT CONFLICTS =====
# TensorFlow uses FetchContent which can conflict with cached builds
set(FETCHCONTENT_QUIET ON)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

# ===== XMORPHY SUBMODULE (с его собственными submodule'ями!) =====
set(XMORPHY_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/XMorphy")
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "XMorphy submodule not found at ${XMORPHY_PROJECT_PATH}")
endif()

# ⚠️ ВАЖНО: XMorphy имеет свои submodule'и в contrib/
# Они загружаются автоматически при git clone --recursive
# Проверяем что они есть
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/contrib/tensorflow")
    message(FATAL_ERROR "XMorphy contrib/tensorflow not found. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${XMORPHY_PROJECT_PATH}/contrib/icu")
    message(FATAL_ERROR "XMorphy contrib/icu not found. Run: git submodule update --init --recursive")
endif()

# Добавляем XMorphy как submodule (её CMakeLists.txt сам подключит свои submodule'и)
add_subdirectory("${XMORPHY_PROJECT_PATH}")
message(STATUS "XMorphy submodule added from: ${XMORPHY_PROJECT_PATH}")

# ===== ТВОЙ ПРОЕКТ =====
# Include directories for project source files
include_directories(
    src/grammar_component
    src/grammar_patterns
    src/phrases_collecting
    src/utils
)

# List of source files in the project
file(GLOB_RECURSE SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/*/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*/*.h
)

include_directories(src)

# Find Boost
find_package(Boost REQUIRED COMPONENTS filesystem program_options)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Boost not found!")
endif()

# Find SQLite3
find_package(SQLite3 REQUIRED)

# === Создаём статическую библиотеку из исходников проекта ===
set(MAIN_CPP "${PROJECT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM SOURCE_FILES "${MAIN_CPP}")
add_library(AutoThematicLib STATIC ${SOURCE_FILES})
target_include_directories(AutoThematicLib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(AutoThematicLib PRIVATE -Wno-unused)

# === Основное приложение ===
add_executable(AutoThematicThesaurus ${MAIN_CPP})
target_link_libraries(AutoThematicThesaurus PRIVATE AutoThematicLib Boost::filesystem Boost::program_options)
target_include_directories(AutoThematicThesaurus PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

# Set installation directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)

# Enable position-independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define necessary preprocessor macros
add_definitions(-DTF_LITE_DISABLE_XNNPACK_DELEGATE)
add_definitions(-DMIN_LOG_LEVEL=4)

# ===== XMORPHY LINKING =====
# XMorphy уже скомпилирована как цель Xmorphy в submodule
# Её CMakeLists.txt уже подключил все зависимости (TensorFlow Lite, ICU, fasttext и т.д.)
target_link_libraries(AutoThematicLib PUBLIC Xmorphy)
target_link_libraries(AutoThematicLib PUBLIC ${SQLite3_LIBRARIES})

# Copy necessary data files to the build directory
file(COPY ${XMORPHY_PROJECT_PATH}/data/dicts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${XMORPHY_PROJECT_PATH}/data/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Include additional directories (если нужны твои ресурсы)
include_directories(${PROJECT_SOURCE_DIR}/my_data)
include_directories(${PROJECT_SOURCE_DIR}/x_morphy_resuls)

# === GoogleTest integration ===
find_package(GTest REQUIRED)

if(GTest_FOUND)
    message(STATUS "GTest found: ${GTest_INCLUDE_DIRS}")
    enable_testing()
    add_subdirectory(tests)
else()
    message(WARNING "GTest not found! Unit tests will be skipped.")
endif()

# === Debug Info ===
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "XMorphy: ${XMORPHY_PROJECT_PATH}")
message(STATUS "Boost: ${Boost_INCLUDE_DIRS}")
message(STATUS "SQLite3: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "=========================")
message(STATUS "")