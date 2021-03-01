# FluffECS

At the heart of this C++17 library is the `flf::World` class: It takes care of saving all of the entities components in contigous memory. It is designed to make iterations through components as fast as possible, arriving at the same speed as iterating over raw vectors.

# Usage
An example of how this library might be used can be found in examples/example.cpp

To use classes as components you can simply add it as a component to an entity using the function `createEntityWith<TComponents>()` or add it to existing entities using `addComponent<TComponents>(entity)`. The only restrictions placed upon component types is to be default, move and copy constructible.
