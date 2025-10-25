# Собираем все тесты
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(UnitTests ${TEST_SOURCES})

# Линкуем с основной библиотекой и GTest
find_package(GTest REQUIRED)
target_link_libraries(UnitTests PRIVATE AutoThematicLib GTest::gtest_main pthread)

target_include_directories(UnitTests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(UnitTests)
