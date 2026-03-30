cmake_minimum_required(VERSION 3.14)
project(HFT_Engine CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Release>:-fno-omit-frame-pointer>  # keeps perf/flamegraph usable
)
add_link_options(
    $<$<CONFIG:Release>:-flto>
)

find_package(Threads REQUIRED)

# packaging core logic into a static library except main.cpp
file(GLOB_RECURSE HFT_SOURCES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)
list(FILTER HFT_SOURCES EXCLUDE REGEX ".*/(main|benchmark)\\.cpp$")

# all those files are compiled into a single package hft_lib
if(HFT_SOURCES)
    add_library(hft_lib STATIC ${HFT_SOURCES})
    target_include_directories(hft_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
endif()

# main executable and linking libs
add_executable(hft_engine src/main.cpp)
target_include_directories(hft_engine PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(HFT_SOURCES)
    target_link_libraries(hft_engine PRIVATE hft_lib Threads::Threads)
endif()

add_executable(run_benchmark src/benchmark.cpp)
target_include_directories(run_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(HFT_SOURCES)
    target_link_libraries(run_benchmark PRIVATE hft_lib Threads::Threads)
else()
    target_link_libraries(run_benchmark PRIVATE Threads::Threads)
endif()

# build google tests alongside
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# testing suite
enable_testing()
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tests/*.cpp)
add_executable(run_tests ${TEST_SOURCES})
target_include_directories(run_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(HFT_SOURCES)
    target_link_libraries(run_tests gtest gtest_main hft_lib)
else()
    target_link_libraries(run_tests gtest gtest_main)
endif()

include(GoogleTest)
gtest_discover_tests(run_tests)