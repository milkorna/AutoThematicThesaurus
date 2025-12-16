file(COPY input DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(AutoThematicThesaurus_tests ${TEST_SOURCES})

find_package(GTest REQUIRED)
target_link_libraries(AutoThematicThesaurus_tests PRIVATE AutoThematicLib GTest::gtest_main pthread)

target_include_directories(AutoThematicThesaurus_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_compile_definitions(AutoThematicThesaurus_tests PRIVATE
    TEST_INPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/input"
)

include(GoogleTest)
gtest_discover_tests(AutoThematicThesaurus_tests)
