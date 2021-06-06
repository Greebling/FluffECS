#include "doctest.h"

#include <FluffECS/DynamicVector.h>


struct Vector3
{
	float x, y, z;
};

struct Quaternion
{
	float x, y, z, w;
};

TEST_CASE_TEMPLATE("Dynamic Vector PushBack<T>", T, Vector3, Quaternion)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};
	flf::internal::DynamicVector vec{res};
	
	CHECK(vec.ByteSize() == 0);
	
	for (std::size_t i = 1; i <= 32; ++i)
	{
		vec.PushBack<T>();
		CHECK(vec.ByteSize() == sizeof(T) * i);
		CHECK(vec.Size<T>() == i);
		CHECK(vec.ByteCapacity() >= sizeof(T) * i);
	}
	
	auto item = T{};
	vec.PushBack(item);
	CHECK(vec.ByteSize() == sizeof(T) * 33);
	CHECK(vec.Size<T>() == 33);
	CHECK(vec.ByteCapacity() >= sizeof(T) * 33);
	CHECK(vec.Capacity<T>() >= 33);
	CHECK(vec.Capacity<T>() <= 128);
}

TEST_CASE_TEMPLATE("Dynamic Vector EmplaceBack<T>", T, Vector3, Quaternion)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};
	flf::internal::DynamicVector vec{res};
	
	CHECK(vec.ByteSize() == 0);
	
	for (std::size_t i = 1; i <= 32; ++i)
	{
		vec.EmplaceBack<T>();
		CHECK(vec.ByteSize() == sizeof(T) * i);
		CHECK(vec.Size<T>() == i);
		CHECK(vec.ByteCapacity() >= sizeof(T) * i);
	}
	
	vec.EmplaceBack(std::forward<T>(T{}));
	CHECK(vec.ByteSize() == sizeof(T) * 33);
	CHECK(vec.Size<T>() == 33);
	CHECK(vec.ByteCapacity() >= sizeof(T) * 33);
	CHECK(vec.Capacity<T>() >= 33);
	CHECK(vec.Capacity<T>() <= 128);
}

TEST_CASE_TEMPLATE("Dynamic Vector Reserve<T>", T, Vector3, Quaternion)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};
	flf::internal::DynamicVector vec{res};
	
	CHECK(vec.ByteSize() == 0);
	
	vec.Reserve<T>(32);
	CHECK(vec.ByteSize() == 0);
	CHECK(vec.Size<T>() == 0);
	CHECK(vec.ByteCapacity() >= sizeof(T) * 32);
	CHECK(vec.Capacity<T>() >= 32);
	CHECK(vec.Capacity<T>() <= 64);
}

TEST_CASE_TEMPLATE("Dynamic Vector Resize<T>", T, Vector3, Quaternion)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};
	flf::internal::DynamicVector vec{res};
	
	CHECK(vec.ByteSize() == 0);
	
	vec.Resize<T>(32);
	CHECK(vec.ByteSize() == sizeof(T) * 32);
	CHECK(vec.Size<T>() == 32);
	CHECK(vec.ByteCapacity() >= sizeof(T) * 32);
	CHECK(vec.Capacity<T>() >= 32);
	CHECK(vec.Capacity<T>() <= 64);
}

TEST_CASE_TEMPLATE("Dynamic Vector ResizeUnsafe<T>", T, Vector3, Quaternion)
{
	std::pmr::unsynchronized_pool_resource res{{8, 1024}};
	flf::internal::DynamicVector vec{res};
	
	CHECK(vec.ByteSize() == 0);
	
	vec.ResizeUnsafe<T>(32);
	CHECK(vec.ByteSize() == sizeof(T) * 32);
	CHECK(vec.Size<T>() == 32);
	CHECK(vec.ByteCapacity() >= sizeof(T) * 32);
	CHECK(vec.Capacity<T>() >= 32);
	CHECK(vec.Capacity<T>() <= 64);
}