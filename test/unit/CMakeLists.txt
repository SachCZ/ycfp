find_package(Catch2 REQUIRED)
add_executable(unit_tests unit_tests_runner.cpp expectation_test.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2 ycfp)