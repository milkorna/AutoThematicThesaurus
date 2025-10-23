file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(UnitTests ${TEST_SOURCES})
target_link_libraries(UnitTests PRIVATE GTest::gtest_main pthread)

# подключение исходников проекта
target_include_directories(UnitTests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(UnitTests)
