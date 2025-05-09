add_executable(RunTests
    TestMain.cpp
    TestComponent.cpp
)

target_link_libraries(RunTests PRIVATE gtest gtest_main)

target_include_directories(RunTests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_test(NAME RunTests COMMAND RunTests)
