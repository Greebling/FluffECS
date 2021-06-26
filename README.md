# FluffECS

At the heart of this C++17 library is the `flf::World` class: It takes care of saving all of the entities components in contigous memory. It is designed to make iterations through components as fast as possible, arriving at the same speed as iterating over raw vectors.

# Example
```c++
#include <FluffECS/World.h>

struct PositionData
{
    float x, y, z;
};

struct VelocityData
{
    float dx, dy, dz;
};

void Update()
{
    flf::World myWorld{};
    myWorld.CreateMultiple(16, PositionData{4.f, 2.f, 0.f}, VelocityData{1.f, 0, 0});
    
    float deltaTime = 1.f / 60.f;
    myWorld.Foreach<PositionData &, VelocityData>(
        [deltaTime](PositionData &position, VelocityData velocity)
        {
            position.x += velocity.dx * deltaTime;
            position.y += velocity.dy * deltaTime;
            position.z += velocity.dz * deltaTime;
        }
    );
}
```
Another example of how this library might be used can be found under examples/example.cpp

# Building
To use FluffECS in your project add the following lines to your CMakeLists.txt:
```cmake
add_subdirectory(PATH_TO_FLUFF_ECS_TOP_FOLDER)
target_link_libraries(YOUR_TARGET PUBLIC FluffECS)
```
