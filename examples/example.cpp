#include "../include/World.h"

struct PositionData
{
	float x,y,z;
};

struct VelocityData
{
	float dx, dy, dz;
};

int main()
{
	int numberOfEntities = 1024;
	fluff:World myWorld;
	myWorld.AddMultiple<PositionData, VelocityData>(numberOfEntities, {4.f,2.f,0.f}, {1.f,0,0});
	
	float deltaTime = 1.f / 60.f;
	myWorld.Foreach<PositionData, VelocityData>(
			[deltaTime](PositionData &position, const VelocityData &velocity)
			{
				position.x += velocity.dx * deltaTime;
				position.y += velocity.dy * deltaTime;
				position.z += velocity.dz * deltaTime;
			}
			);
	
	fluff:EntityWith<PositionData, VelocityData> addedEntity = myWorld.Add();
	PositionData &position = myWorld.Get<PositionData>(addedEntity);
	myWorld.Remove(addedEntity);
}
