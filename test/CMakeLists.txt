add_executable(FluffECSTest
        doctest.h
        TestDefinition.cpp
        TestDynamicVector.cpp
        TestWorld.cpp)


target_compile_options(FluffECSTest PRIVATE -Wall -std=c++17)
