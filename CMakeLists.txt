cmake_minimum_required(VERSION 3.17)
project(FluffECS
        VERSION 0.1
        DESCRIPTION "A fast header only ECS Library"
        LANGUAGES CXX)

add_library(FluffECS INTERFACE)

target_compile_options(FluffECS INTERFACE -Wall)

target_include_directories(FluffECS INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
    target_link_libraries(FluffECSTest FluffECS)
endif()