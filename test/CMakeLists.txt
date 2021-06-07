add_executable(FluffECSTEST
        doctest.h
        TestDefinition.cpp
        TestDynamicVector.cpp
       )


target_compile_options(FluffECSTEST PRIVATE -Wall -std=c++20)
