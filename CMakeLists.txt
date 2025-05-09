cmake_minimum_required(VERSION 3.10)
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

# Add Google Test
find_package(GTest REQUIRED)

# Ensure compatibility with modern CMake policies
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()

# Include directories for project source files
include_directories(
    src/grammar_component
    src/grammar_patterns
    src/phrases_collecting
    src/utils
)

# List of source files in the project
set(SOURCE_FILES
  src/grammar_component/Component.h
  src/grammar_component/GrammarCondition.cpp
  src/grammar_component/GrammarCondition.h
  src/grammar_component/WordComponent.cpp
  src/grammar_component/WordComponent.h
  src/grammar_component/ModelComponent.cpp
  src/grammar_component/ModelComponent.h
  src/grammar_patterns/PatternParser.cpp
  src/grammar_patterns/PatternParser.h
  src/grammar_patterns/GrammarPatternManager.cpp
  src/grammar_patterns/GrammarPatternManager.h

  src/phrases_collecting/PhrasesStorageLoader.h
  src/phrases_collecting/Embedding.cpp
  src/phrases_collecting/Embedding.h
  src/phrases_collecting/PatternPhrasesStorage.cpp
  src/phrases_collecting/PatternPhrasesStorage.h
  src/phrases_collecting/PhrasesCollectorUtils.cpp
  src/phrases_collecting/PhrasesCollectorUtils.h
  src/phrases_collecting/SimplePhrasesCollector.cpp
  src/phrases_collecting/SimplePhrasesCollector.h
  src/phrases_collecting/ComplexPhrasesCollector.cpp
  src/phrases_collecting/ComplexPhrasesCollector.h
  src/phrases_collecting/WordComplex.cpp
  src/phrases_collecting/WordComplex.h

  src/utils/OutputRedirector.h
  src/utils/SemanticRelations.cpp
  src/utils/SemanticRelations.h
  src/utils/ThreadController.h
  src/utils/TextCorpus.cpp
  src/utils/TextCorpus.h
  src/utils/Logger.cpp
  src/utils/Logger.h
  src/utils/TokenizedSentenceCorpus.cpp
  src/utils/TokenizedSentenceCorpus.h
  src/utils/StringFilters.cpp
  src/utils/StringFilters.h
  src/utils/LSA.h
  src/utils/LSA.cpp
  src/utils/TermLSA.h
  src/utils/TermLSA.cpp
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

# Create the main executable
add_executable(AutoThematicThesaurus src/main.cpp ${SOURCE_FILES})
target_compile_options(AutoThematicThesaurus PRIVATE -Wno-unused -Werror)

add_subdirectory(src/tests)

# Link Boost libraries
target_link_libraries(AutoThematicThesaurus PUBLIC Boost::filesystem Boost::program_options)

target_include_directories(AutoThematicThesaurus PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_compile_options(-w)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

# Set installation directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)

# Define paths for dependencies
get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
set(XMORPHY_PROJECT_PATH "${PARENT_DIR}/XMorphy" CACHE PATH "Path to the XMorphy project" FORCE)
set(XMorphy_INCLUDE_DIR "${XMORPHY_PROJECT_PATH}/src")

# Enable position-independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define necessary preprocessor macros
add_definitions(-DTF_LITE_DISABLE_XNNPACK_DELEGATE)
add_definitions(-DMIN_LOG_LEVEL=4)

# Configure TensorFlow Lite
set(TENSORFLOW_SOURCE_DIR "" CACHE PATH "Directory that contains the TensorFlow project")
if(NOT TENSORFLOW_SOURCE_DIR)
  get_filename_component(TENSORFLOW_SOURCE_DIR "${XMORPHY_PROJECT_PATH}/contrib/tensorflow" ABSOLUTE)
endif()
add_subdirectory("${TENSORFLOW_SOURCE_DIR}/tensorflow/lite" "${CMAKE_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)
SET(TFLITE_INCLUDE_DIRS "${TFLITE_INCLUDE_DIRS}" "${TENSORFLOW_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/flatbuffers/include")

# Configure ICU support
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(ICU_ROOT_DIR "${XMORPHY_PROJECT_PATH}/contrib/icu")
set(ICU_ROOT "${XMORPHY_PROJECT_PATH}/contrib/icu")
include(${PROJECT_SOURCE_DIR}/cmake/icu.cmake)
find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)
if(ICU_FOUND)
  target_link_libraries(AutoThematicThesaurus PUBLIC ICU::ICU)
endif(ICU_FOUND)

# Copy necessary data files to the build directory
file(COPY ${XMORPHY_PROJECT_PATH}/data/dicts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${XMORPHY_PROJECT_PATH}/data/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Include XMorphy dependency
add_subdirectory(${XMORPHY_PROJECT_PATH}/contrib ${CMAKE_CURRENT_BINARY_DIR}/xmorphy-contrib)
set(XMORPHY_SOURCE_DIR "${XMORPHY_PROJECT_PATH}/src/xmorphy")
file(GLOB_RECURSE XMORPHY_SOURCES "${XMORPHY_SOURCE_DIR}/*.cpp" "${XMORPHY_SOURCE_DIR}/*.h")
add_library(Xmorphy STATIC ${XMORPHY_SOURCES})

message("
  ${frugally-deep_INCLUDE_DIR}
  ${Eigen3_INCLUDE_DIR}
  ${FunctionalPlus_INCLUDE_DIR}
  ${nlohmann_json_INCLUDE_DIR}
  ${fasttext_INCLUDE_DIR}
  ${incbin_INCLUDE_DIR}" )

# Find and link SQLite3
find_package(SQLite3 REQUIRED)

# Configure include directories
target_include_directories (Xmorphy SYSTEM PUBLIC
  ${XMORPHY_PROJECT_PATH}/src
  ${MIMALLOC_INCLUDE_DIR}
  ${TFLITE_INCLUDE_DIRS}
  ${BOOST_HEADERONLY_INCLUDE_DIR}
  ${frugally-deep_INCLUDE_DIR}
  ${Eigen3_INCLUDE_DIR}
  ${FunctionalPlus_INCLUDE_DIR}
  ${nlohmann_json_INCLUDE_DIR}
  ${fasttext_INCLUDE_DIR}
  ${incbin_INCLUDE_DIR}
  ${XMORPHY_PROJECT_PATH}/contrib/tabulate/include/tabulate
  PRIVATE ${ICU_INCLUDE_DIR}
)

# Include additional directories
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${frugally-deep_INCLUDE_DIR}
  ${Eigen3_INCLUDE_DIR}
  ${FunctionalPlus_INCLUDE_DIR}
  ${nlohmann_json_INCLUDE_DIR}
  ${fasttext_INCLUDE_DIR}
  ${incbin_INCLUDE_DIR}
  ${XMORPHY_PROJECT_PATH}/contrib/tabulate/include
  ${SQLite3_INCLUDE_DIRS}
)

# Include paths for additional resources
include_directories(${PROJECT_SOURCE_DIR}/my_data)
include_directories(${PROJECT_SOURCE_DIR}/x_morphy_resuls)

# Define XMorphy static library path
set(XMORPHY_LIBRARY "${XMORPHY_PROJECT_PATH}/build/temp.linux-x86_64-3.10/libXmorphy.a")
include_directories(${XMORPHY_PROJECT_PATH}/src)

# Link necessary libraries
target_link_libraries(AutoThematicThesaurus PRIVATE ${XMORPHY_LIBRARY} ${SQLite3_LIBRARIES})
target_compile_options(tensorflow-lite PRIVATE -Wno-unused -Wno-error -DNDEBUG -w)
target_link_libraries(AutoThematicThesaurus PUBLIC ${ICU_LIBRARIES} PRIVATE fasttext-static_pic tabulate::tabulate tensorflow-lite -ldl -lpthread -lstdc++)

# Ensure XMorphy library exists
if(NOT EXISTS ${XMORPHY_LIBRARY})
  message(FATAL_ERROR "XMorphy library not found: ${XMORPHY_LIBRARY}")
endif()
