#include "doctest.h"

#include <FluffECS/World.h>

struct Vector3 {
    int x, y, z;

    bool operator==(const Vector3 vec) const {
        return vec.x == x && vec.y == y && vec.z == z;
    }
};


struct MoveDetectingVector3 {
    int x, y, z;
    bool wasMoved = false;

    MoveDetectingVector3() = default;

    MoveDetectingVector3(int x, int y, int z) : x(x), y(y), z(z) {
    }

    MoveDetectingVector3(const MoveDetectingVector3 &t) = default;

    MoveDetectingVector3(MoveDetectingVector3 &&t) noexcept: x(t.x), y(t.y), z(t.z) {
        t.wasMoved = true;
    }


    bool operator==(const Vector3 vec) const {
        return vec.x == x && vec.y == y && vec.z == z;
    }
};

struct Quaternion {
    float x, y, z, w;
};

struct Empty {
};

TEST_CASE("World CreateEntity")
{
    flf::World myWorld{};
    MoveDetectingVector3 baseVec = {1, 2, 3};
    const MoveDetectingVector3 &vecRef = baseVec;


    myWorld.CreateEntity(baseVec);
    CHECK_FALSE(baseVec.wasMoved);
    myWorld.CreateEntity(vecRef);
    CHECK_FALSE(baseVec.wasMoved);
    myWorld.CreateEntity(MoveDetectingVector3{1, 2, 3});
    myWorld.CreateEntity(std::move(baseVec));
    CHECK(baseVec.wasMoved);
}

TEST_CASE("World CreateMultiple")
{
    flf::World myWorld{};
    Vector3 baseVec = {1, 2, 3};


    myWorld.CreateMultiple<Vector3>(8);
    myWorld.CreateMultiple(8, Vector3{5, 6, 7});
    myWorld.CreateMultiple(8, baseVec);

    {
        std::size_t countDefaultConstruct = 0;
        std::size_t countReferenceConstruct = 0;
        std::size_t countForwardConstruct = 0;

        myWorld.Foreach([&](Vector3 vec) {
            if (vec == Vector3()) {
                countDefaultConstruct++;
            } else if (vec == baseVec) {
                countReferenceConstruct++;
            } else if (vec == Vector3{5, 6, 7}) {
                countForwardConstruct++;
            } else {
                CHECK(false);
            }
        });
        CHECK_EQ(countDefaultConstruct, 8);
        CHECK_EQ(countReferenceConstruct, 8);
        CHECK_EQ(countForwardConstruct, 8);
    }

    myWorld.CreateMultiple<Vector3, Quaternion>(16);
    myWorld.CreateMultiple<Quaternion>(16);
    {
        std::size_t countDefaultConstruct = 0;
        std::size_t countReferenceConstruct = 0;
        std::size_t countForwardConstruct = 0;

        myWorld.Foreach([&](Vector3 vec) {
            if (vec == Vector3()) {
                countDefaultConstruct++;
            } else if (vec == baseVec) {
                countReferenceConstruct++;
            } else if (vec == Vector3{5, 6, 7}) {
                countForwardConstruct++;
            } else {
                CHECK(false);
            }
        });
        CHECK_EQ(countDefaultConstruct, 8 + 16);
        CHECK_EQ(countReferenceConstruct, 8);
        CHECK_EQ(countForwardConstruct, 8);
    }
}

TEST_CASE("World Foreach Value Types")
{
    flf::World myWorld{};

    std::vector<flf::Entity> createdEntities{};
    createdEntities.reserve(32);
    for (std::size_t i = 0; i < 16; ++i) {
        createdEntities.push_back(myWorld.CreateEntity(int(i), Quaternion{}));
    }
    for (std::size_t i = 16; i < 32; ++i) {
        createdEntities.push_back(myWorld.CreateEntity(int(i)));
    }


    std::array<bool, 16> hasAllQuaternionAndVector{};
    myWorld.Foreach(
            [&](int val, Quaternion quaternion) {
                hasAllQuaternionAndVector[val] = true;
            });
    for (const auto item : hasAllQuaternionAndVector) {
        CHECK(item);
    }


    std::array<bool, 32> hasAllVector{};
    myWorld.Foreach(
            [&](int val) {
                hasAllVector[val] = true;
            });
    for (const auto item : hasAllVector) {
        CHECK(item);
    }
}

TEST_CASE("World Foreach Reference")
{
    flf::World myWorld{};
    std::vector<flf::Entity> createdEntities{};
    createdEntities.reserve(32);
    for (std::size_t i = 0; i < 16; ++i) {
        auto iInt = int(i);
        createdEntities.push_back(myWorld.CreateEntity(Vector3{iInt, iInt * 2, iInt * 4}, Quaternion{}));
    }

    myWorld.Foreach(
            [](Vector3 &vec) {
                vec.y /= 2;
            });
    myWorld.Foreach(
            [](Vector3 vec) {
                CHECK_EQ(vec.x, vec.y);
            });
}

TEST_CASE("World Foreach With Empty Type")
{
    flf::World myWorld{};
    std::vector<flf::Entity> createdEntities{};
    createdEntities.reserve(32);
    for (std::size_t i = 0; i < 16; ++i) {
        auto iInt = int(i);
        createdEntities.push_back(myWorld.CreateEntity(Vector3{iInt, iInt * 2, iInt * 4}, Quaternion{}, Empty{}));
    }

    myWorld.Foreach(
            [](Vector3 &vec, Quaternion quaternion, Empty) {
                vec.y /= 2;
            });
    myWorld.Foreach(
            [](Vector3 vec) {
                CHECK_EQ(vec.x, vec.y);
            });
}

TEST_CASE("World ForeachEntity Value Types")
{
    flf::World myWorld{};

    std::vector<flf::Entity> createdEntities{};
    createdEntities.reserve(32);
    for (std::size_t i = 0; i < 16; ++i) {
        createdEntities.push_back(myWorld.CreateEntity(int(i), Quaternion{}));
    }
    for (std::size_t i = 16; i < 32; ++i) {
        createdEntities.push_back(myWorld.CreateEntity(int(i)));
    }


    std::array<bool, 16> hasAllQuaternionAndVector{};
    myWorld.ForeachEntity(
            [&](flf::EntityId, int val, Quaternion quaternion) {
                hasAllQuaternionAndVector[val] = true;
            });
    for (const auto item : hasAllQuaternionAndVector) {
        CHECK(item);
    }


    std::array<bool, 32> hasAllVector{};
    myWorld.ForeachEntity(
            [&](flf::EntityId, int val) {
                hasAllVector[val] = true;
            });
    for (const auto item : hasAllVector) {
        CHECK(item);
    }
}

TEST_CASE("World Foreach const Value Types")
{
	flf::World myWorld{};
	
	std::vector<flf::Entity> createdEntities{};
	createdEntities.reserve(32);
	for (std::size_t i = 0; i < 16; ++i) {
		createdEntities.push_back(myWorld.CreateEntity(int(i), Quaternion{}));
	}
	for (std::size_t i = 16; i < 32; ++i) {
		createdEntities.push_back(myWorld.CreateEntity(int(i)));
	}
	
	
	std::array<bool, 16> hasAllQuaternionAndVector{};
	myWorld.Foreach(
			[&](const int val, const Quaternion quaternion) {
				hasAllQuaternionAndVector[val] = true;
			});
	for (const auto item : hasAllQuaternionAndVector) {
		CHECK(item);
	}
	
	
	std::array<bool, 32> hasAllVector{};
	myWorld.Foreach(
			[&](const int val) {
				hasAllVector[val] = true;
			});
	for (const auto item : hasAllVector) {
		CHECK(item);
	}
}

TEST_CASE("World ForeachEntity With Empty Type")
{
    flf::World myWorld{};
    std::vector<flf::Entity> createdEntities{};
    createdEntities.reserve(32);
    for (std::size_t i = 0; i < 16; ++i) {
        auto iInt = int(i);
        createdEntities.push_back(myWorld.CreateEntity(Vector3{iInt, iInt * 2, iInt * 4}, Quaternion{}, Empty{}));
    }

    myWorld.ForeachEntity(
            [](flf::EntityId, Vector3 &vec, Quaternion quaternion, Empty) {
                vec.y /= 2;
            });
    myWorld.ForeachEntity(
            [](flf::EntityId, Vector3 vec) {
                CHECK_EQ(vec.x, vec.y);
            });
}

TEST_CASE("Get all non-empty types")
{
    std::size_t size0 = decltype(
    flf::internal::AllNonEmptyTypes(flf::internal::TypeList<Empty>()))::Size();
    CHECK(size0 == 0);

    constexpr std::size_t size1 = decltype(
    flf::internal::AllNonEmptyTypes(flf::internal::TypeList<Vector3, Empty>()))::Size();
    CHECK(size1 == 1);

    static_assert(std::is_same_v<flf::internal::FirstNonEmpty<Vector3>, Vector3>);
}

TEST_CASE("Print types")
{
	flf::internal::PrintTypes(flf::internal::TypeList<Vector3, Quaternion>());
	auto lmb = [](const int &, float)
			{
		return true;
			};
	PrintTypes(flf::internal::CallableArgList(lmb));
}