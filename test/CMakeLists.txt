add_executable(FluffECSTest
        doctest.h
        TestDefinition.cpp
        TestDynamicVector.cpp
       )


target_compile_options(FluffECSTest PRIVATE -Wall -std=c++20)
