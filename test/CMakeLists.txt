add_executable(FluffECSTEST
        doctest.h
        TestDynamicVector.cpp)


target_compile_options(FluffECSTEST PRIVATE -Wall -std=c++20)
