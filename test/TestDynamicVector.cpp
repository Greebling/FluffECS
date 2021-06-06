#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <FluffECS/DynamicVector.h>


struct Vector3
{
	float x, y, z;
};

TEST_CASE_TEMPLATE("testing Dynamic Vector PushBack<T>", T, Vector3)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};;
	flf::internal::DynamicVector vec{res};
	CHECK(vec.Size<T>() == 0);
	vec.PushBack<T>();
	CHECK(vec.Size<T>() == 1);
	CHECK(vec.ByteSize() == sizeof(T));
}