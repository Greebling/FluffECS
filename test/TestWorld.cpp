#include "doctest.h"

#include <FluffECS/World.h>

struct Vector3
{
	float x, y, z;
	
	bool operator==(const Vector3 vec) const
	{
		return vec.x == x && vec.y == y && vec.z == z;
	}
};


struct MoveDetectingVector3
{
	float x, y, z;
	bool wasMoved = false;
	
	MoveDetectingVector3()
	{
	}
	
	MoveDetectingVector3(float x, float y, float z) : x(x), y(y), z(z)
	{
	}
	
	MoveDetectingVector3(const MoveDetectingVector3 &t)  = default;
	
	MoveDetectingVector3(MoveDetectingVector3 && t)  noexcept : x(t.x), y(t.y), z(t.z)
	{
		t.wasMoved = true;
	}
	
	
	bool operator==(const Vector3 vec) const
	{
		return vec.x == x && vec.y == y && vec.z == z;
	}
};

struct Quaternion
{
	float x, y, z, w;
};

TEST_CASE("World CreateEntity")
{
	flf::World myWord{};
	MoveDetectingVector3 baseVec = {1, 2, 3};
	const MoveDetectingVector3 &vecRef = baseVec;
	
	
	myWord.CreateEntity(baseVec);
	CHECK_FALSE(baseVec.wasMoved);
	myWord.CreateEntity(vecRef);
	CHECK_FALSE(baseVec.wasMoved);
	myWord.CreateEntity(MoveDetectingVector3{1, 2, 3});
	myWord.CreateEntity(std::move(baseVec));
	CHECK(baseVec.wasMoved);
}

TEST_CASE("World CreateMultiple")
{
	flf::World myWord{};
	Vector3 baseVec = {1, 2, 3};
	
	
	myWord.CreateMultiple<Vector3>(8);
	myWord.CreateMultiple(8, Vector3{5, 6, 7});
	myWord.CreateMultiple(8, baseVec);
	
	{
		std::size_t countDefaultConstruct = 0;
		std::size_t countReferenceConstruct = 0;
		std::size_t countForwardConstruct = 0;
		
		myWord.Foreach<Vector3>([&](Vector3 vec)
		                        {
			                        if (vec == Vector3())
			                        {
				                        countDefaultConstruct++;
			                        } else if (vec == baseVec)
			                        {
				                        countReferenceConstruct++;
			                        } else if (vec == Vector3{5, 6, 7})
			                        {
				                        countForwardConstruct++;
			                        } else
			                        {
				                        CHECK(false);
			                        }
		                        });
		CHECK_EQ(countDefaultConstruct, 8);
		CHECK_EQ(countReferenceConstruct, 8);
		CHECK_EQ(countForwardConstruct, 8);
	}
	
	myWord.CreateMultiple<Vector3, Quaternion>(16);
	myWord.CreateMultiple<Quaternion>(16);
	{
		std::size_t countDefaultConstruct = 0;
		std::size_t countReferenceConstruct = 0;
		std::size_t countForwardConstruct = 0;
		
		myWord.Foreach<Vector3>([&](Vector3 vec)
		                        {
			                        if (vec == Vector3())
			                        {
				                        countDefaultConstruct++;
			                        } else if (vec == baseVec)
			                        {
				                        countReferenceConstruct++;
			                        } else if (vec == Vector3{5, 6, 7})
			                        {
				                        countForwardConstruct++;
			                        } else
			                        {
				                        CHECK(false);
			                        }
		                        });
		CHECK_EQ(countDefaultConstruct, 8 + 16);
		CHECK_EQ(countReferenceConstruct, 8);
		CHECK_EQ(countForwardConstruct, 8);
	}
}