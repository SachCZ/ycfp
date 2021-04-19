cmake_minimum_required(VERSION 3.17)
project(ycfp)

set(CMAKE_CXX_STANDARD 17)

add_library(ycfp src/expectation.cpp src/ycfp.cpp)
find_package(yaml-cpp)
target_link_libraries(ycfp yaml-cpp)
target_include_directories(ycfp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ycfp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(test/unit)
add_subdirectory(test/integration)
