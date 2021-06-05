#include <iostream>
#include <FluffECS/World.h> // including this file is enough to give you the full functionality of FluffECS

struct PositionData
{
	float x, y, z;
};

struct VelocityData
{
	float dx, dy, dz;
};

int main()
{
	flf::World myWorld{};
	
	// create many entities with the same components at once
	int numberOfEntities = 1024;
	myWorld.CreateMultiple<PositionData, VelocityData>(numberOfEntities, {4.f, 2.f, 0.f}, {1.f, 0, 0});
	
	// iterate over all entities with specific components
	float deltaTime = 1.f / 60.f;
	myWorld.Foreach<PositionData &, VelocityData>(
			// both reference and value semantics are supported, giving the compiler additional possibilities to optimize
			[deltaTime](PositionData &position, VelocityData velocity)
			{
				position.x += velocity.dx * deltaTime;
				position.y += velocity.dy * deltaTime;
				position.z += velocity.dz * deltaTime;
			}
	);
	
	// can also get the entities ID for additional information
	myWorld.ForeachEntity<PositionData&>(
			// using auto types for parameters is of course also possible
			[](flf::EntityId id, auto position)
			{
				position.x += ((float) id) * 0.01f;
			}
			);
	
	
	
	flf::Entity addedEntity = myWorld.CreateEntity<PositionData>();
	if(PositionData* position = addedEntity.Get<PositionData>(); position != nullptr)
	{
		std::cout << "Entity is at " << position->x << " | " << position->y << " | " << position->z << "\n";
	}
	
	addedEntity.Destroy(); // destroys the entity and all of its components
}
