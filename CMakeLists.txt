cmake_minimum_required(VERSION 3.17)
project(FluffECS
        VERSION 0.1
        DESCRIPTION "A fast header only ECS Library"
        LANGUAGES CXX)

add_library(FluffECS INTERFACE)

target_compile_options(FluffECS INTERFACE -Wall)

target_sources(FluffECS
        INTERFACE
        include/FluffECS/Keywords.h
        include/FluffECS/TypeId.h
        include/FluffECS/TypeList.h
        include/FluffECS/SortedKeyTree.h
        include/FluffECS/VirtualConstructor.h
        include/FluffECS/Entity.h
        include/FluffECS/SparseSet.h
        include/FluffECS/DynamicVector.h
        include/FluffECS/WorldInternal.h
        include/FluffECS/ComponentContainer.h
        include/FluffECS/World.h
        include/FluffECS/Serialization.h
        )

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
    target_include_directories(FluffECSTEST PRIVATE include)
endif()