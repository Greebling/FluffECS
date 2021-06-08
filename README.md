# FluffECS

At the heart of this C++17 library is the `flf::World` class: It takes care of saving all of the entities components in contigous memory. It is designed to make iterations through components as fast as possible, arriving at the same speed as iterating over raw vectors.

# Usage Usage
```c++
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
    myWorld.CreateMultiple(16, PositionData{4.f, 2.f, 0.f}, VelocityData{1.f, 0, 0});
    
    myWorld.Foreach<PositionData &, VelocityData>(
        // both reference and value semantics are supported, giving the compiler additional possibilities to optimize
        [deltaTime](PositionData &position, VelocityData velocity)
        {
          position.x += velocity.dx * deltaTime;
          position.y += velocity.dy * deltaTime;
          position.z += velocity.dz * deltaTime;
        }
    );
}
```
Another example of how this library might be used can be found in examples/example.cpp

# Building
To use FluffECS in your project you add the following lines to your CMakeLists.txt (your path to the FluffECS folder may vary, though):
```cmake
add_subdirectory(libs/FluffECS)
target_include_directories(YOUR_TARGET PUBLIC libs/FluffECS/include)
target_link_libraries(YOUR_TARGET PUBLIC FluffECS)
```
