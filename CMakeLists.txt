cmake_minimum_required(VERSION 3.10)
project(AutoThematicThesaurus VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Enable C++ standard propagation to Abseil" FORCE)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -ffast-math -march=native -fno-omit-frame-pointer")
set (CMAKE_CXX_FLAGS                     "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS} ${PLATFORM_EXTRA_CXX_FLAG} ${COMMON_WARNING_FLAGS} ${CXX_WARNING_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 ${CMAKE_CXX_FLAGS_ADD}")
set (CMAKE_CXX_FLAGS_DEBUG               "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -ggdb3 -fno-inline ${CMAKE_CXX_FLAGS_ADD}")
set (CMAKE_C_FLAGS                       "${CMAKE_C_FLAGS} ${COMPILER_FLAGS} ${COMMON_WARNING_FLAGS} ${CMAKE_C_FLAGS_ADD} -ffast-math -march=native -fno-omit-frame-pointer")
set (CMAKE_C_FLAGS_RELWITHDEBINFO        "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O3 ${CMAKE_C_FLAGS_ADD}")
set (CMAKE_C_FLAGS_DEBUG                 "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -ggdb3 -fno-inline ${CMAKE_C_FLAGS_ADD}")

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()

set(SOURCE_FILES
src/GrammarComponent.cpp
src/GrammarCondition.cpp
src/GrammarPatternManager.cpp
src/ComplexPhrasesCollector.cpp
src/PatternParser.cpp
src/PatternPhrasesStorage.cpp
src/SimplePhrasesCollector.cpp
src/Logger.cpp
src/PhrasesCollectorUtils.cpp

src/GrammarComponent.h
src/GrammarCondition.h
src/GrammarPatternManager.h
src/ComplexPhrasesCollector.h
src/PatternParser.h
src/PatternPhrasesStorage.h
src/SimplePhrasesCollector.h
src/Logger.h
src/PhrasesCollectorUtils.h
)


include_directories(src)

add_executable(AutoThematicThesaurus src/main.cpp ${SOURCE_FILES})
target_compile_options(AutoThematicThesaurus PRIVATE -Wno-unused -Werror)

target_include_directories(AutoThematicThesaurus PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_compile_options(-w) 


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)

get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
set(XMORPHY_PROJECT_PATH "${PARENT_DIR}/XMorphy" CACHE PATH "Path to the XMorphy project" FORCE)
set(XMorphy_INCLUDE_DIR "${XMORPHY_PROJECT_PATH}/src")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_definitions(-DTF_LITE_DISABLE_XNNPACK_DELEGATE)
add_definitions(-DMIN_LOG_LEVEL=2)

set(TENSORFLOW_SOURCE_DIR "" CACHE PATH
  "Directory that contains the TensorFlow project")

if(NOT TENSORFLOW_SOURCE_DIR)
  get_filename_component(TENSORFLOW_SOURCE_DIR
    "${XMORPHY_PROJECT_PATH}/contrib/tensorflow" ABSOLUTE)
endif()

add_subdirectory("${TENSORFLOW_SOURCE_DIR}/tensorflow/lite" "${CMAKE_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)
SET(TFLITE_INCLUDE_DIRS "${TFLITE_INCLUDE_DIRS}" "${TENSORFLOW_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/flatbuffers/include")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(ICU_ROOT_DIR "${XMORPHY_PROJECT_PATH}/contrib/icu")
set(ICU_ROOT "${XMORPHY_PROJECT_PATH}/contrib/icu")
include(${PROJECT_SOURCE_DIR}/cmake/icu.cmake)
find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)
if(ICU_FOUND)
  target_link_libraries(AutoThematicThesaurus PUBLIC ICU::ICU)
endif(ICU_FOUND)

file(COPY ${XMORPHY_PROJECT_PATH}/data/dicts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${XMORPHY_PROJECT_PATH}/data/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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


target_include_directories (Xmorphy SYSTEM PUBLIC ${XMORPHY_PROJECT_PATH}/src ${MIMALLOC_INCLUDE_DIR} ${TFLITE_INCLUDE_DIRS}
    ${BOOST_HEADERONLY_INCLUDE_DIR}
    PRIVATE ${ICU_INCLUDE_DIR})


target_include_directories(Xmorphy SYSTEM PUBLIC
  ${frugally-deep_INCLUDE_DIR}
  ${Eigen3_INCLUDE_DIR}
  ${FunctionalPlus_INCLUDE_DIR}
  ${nlohmann_json_INCLUDE_DIR}
  ${fasttext_INCLUDE_DIR}
  ${incbin_INCLUDE_DIR}
  ${XMORPHY_PROJECT_PATH}/contrib/tabulate/include/tabulate
  )

include_directories(${PROJECT_SOURCE_DIR}/src
  ${frugally-deep_INCLUDE_DIR}
  ${Eigen3_INCLUDE_DIR}
  ${FunctionalPlus_INCLUDE_DIR}
  ${nlohmann_json_INCLUDE_DIR}
  ${fasttext_INCLUDE_DIR}
  ${incbin_INCLUDE_DIR}
  ${XMORPHY_PROJECT_PATH}/contrib/tabulate/include
    )
include_directories(${PROJECT_SOURCE_DIR}/my_data)
include_directories(${PROJECT_SOURCE_DIR}/x_morphy_resuls)

set(XMORPHY_LIBRARY "${XMORPHY_PROJECT_PATH}/build/temp.linux-x86_64-3.10/libXmorphy.a")

include_directories(${XMORPHY_PROJECT_PATH}/src)

target_link_libraries(AutoThematicThesaurus PRIVATE ${XMORPHY_LIBRARY})

target_compile_options(tensorflow-lite PRIVATE -Wno-unused -Wno-error -DNDEBUG-w)
target_link_libraries(AutoThematicThesaurus PUBLIC ${ICU_LIBRARIES} PRIVATE fasttext-static_pic tabulate::tabulate tensorflow-lite -ldl -lpthread -lstdc++)

if(NOT EXISTS ${XMORPHY_LIBRARY})
  message(FATAL_ERROR "XMorphy library not found: ${XMORPHY_LIBRARY}")
endif()

