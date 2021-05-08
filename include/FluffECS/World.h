#ifndef FLUFF_ECS_WORLD_H
#define FLUFF_ECS_WORLD_H

#include <unordered_map>
#include <vector>
#include <array>
#include <memory_resource>
#include <algorithm>

#include "Keywords.h"
#include "TypeId.h"
#include "SortedKeyTree.h"
#include "Entity.h"
#include "TypeList.h"
#include "ComponentContainer.h"
#include "IteratorContainer.h"

namespace flf
{
	/// A world contains many entities that may have differing types. Each entity is saved in a ComponentContainer that has
	/// exactly the specific component types of that entity.
	/// \tparam TMemResource polymorphic memory resource (from std::pmr) used for general storage of components
	template<typename TMemResource = std::pmr::unsynchronized_pool_resource>
	class BasicWorld : private internal::WorldInternal
	{
		static_assert(std::is_base_of_v<std::pmr::memory_resource, TMemResource>, "Memory resource must inherit from std::pmr::memory_resource");
	
	private:
		/// standard array size is 4KB to most efficiently use caching effects
		static constexpr std::size_t COMPONENT_VECTOR_BYTE_SIZE = 4096;
		
		template<typename Key, typename Value>
		using Map = std::unordered_map<Key, Value>;
		
		template<typename ...TComponents>
		using Iterator = ComponentContainer::Iterator<TComponents...>;
		
		static constexpr std::pmr::pool_options STANDARD_POOL_OPTIONS = {8, COMPONENT_VECTOR_BYTE_SIZE};
	
	public:
		~BasicWorld() FLUFF_NOEXCEPT
		{
			for(std::pair<const MultiIdType, ComponentContainer *> container : _componentContainers)
			{
				// we only call the destructor, the freeing of memory happens through memory resources
				container.second->~ComponentContainer();
			}
		}
	
	public:
		/// Iterates over all components of the given types.
		/// \tparam TComponents Entity need to have at minimum to be iterated over
		/// \param function to apply on them
		template<typename ...TComponents, typename TFunc>
		void Foreach(TFunc &&function) noexcept(std::is_nothrow_invocable_v<TFunc>)
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			IteratorContainer<TComponents...> begin = EntitiesWith<TComponents...>().begin();
			for(flf::EntityId i = 0; i < begin.Size(); ++i)
			{
				auto iter = begin.iterators[i];
				
				while(!iter.IsOverEnd())
				{
					auto tup = iter.GetReference();
					function(std::get<TComponents &>(tup) ...);
					++iter;
				}
			}
		}
		
		/// Iterates over all components of the given types.
		/// \tparam TComponents Entity need to have at minimum to be iterated over
		/// \param function to apply on them
		template<typename ...TComponents, typename TFunc>
		void Foreach(TFunc &&function) const noexcept(std::is_nothrow_invocable_v<TFunc>)
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			IteratorContainer<const TComponents...> begin = EntitiesWith<TComponents...>().begin();
			for(flf::EntityId i = 0; i < begin.Size(); ++i)
			{
				auto iter = begin.iterators[i];
				
				while(!iter.IsOverEnd())
				{
					const auto tup = iter.GetReference();
					function(std::get<const TComponents &>(tup) ...);
					++iter;
				}
			}
		}
		
		/// Iterates over all components of the given types.
		/// \tparam TComponents Entity need to have at minimum to be iterated over
		/// \param function to apply on them. Must be in the form identifier(flf::EntityId, TComponents...)
		/// where the first argument is the id of the entity currently iterated
		template<typename ...TComponents, typename TFunc>
		void ForeachEntity(TFunc &&function) noexcept(std::is_nothrow_invocable_v<TFunc>)
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			auto entities = EntitiesWith<TComponents...>();
			IteratorContainer<TComponents...> begin = entities.begin();
			
			for(flf::EntityId i = 0; i < begin.Size(); ++i)
			{
				auto iter = begin.iterators[i];
				
				while(!iter.IsOverEnd())
				{
					auto tup = iter.GetEntityReference();
					function(std::get<0>(tup), std::get<TComponents &>(tup) ...); // note the get<0> for the id
					++iter;
				}
			}
		}
		
		/// Iterates over all components of the given types.
		/// \tparam TComponents Entity need to have at minimum to be iterated over
		/// \param function to apply on them. Must be in the form identifier(flf::EntityId, TComponents...)
		/// where the first argument is the id of the entity currently iterated
		template<typename ...TComponents, typename TFunc>
		void ForeachEntity(TFunc &&function) const noexcept(std::is_nothrow_invocable_v<TFunc>)
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			IteratorContainer<const TComponents...> begin = EntitiesWith<TComponents...>().begin();
			for(flf::EntityId i = 0; i < begin.Size(); ++i)
			{
				auto iter = begin.iterators[i];
				
				while(!iter.IsOverEnd())
				{
					const auto tup = iter.GetEntityReference();
					function(std::get<0>(tup), std::get<const TComponents &>(tup) ...); // note the get<0> for the id
					++iter;
				}
			}
		}
		
		/// Gets the component of a given entity
		/// \tparam TComponent ComponentType to get
		/// \param entity that owns the wanted component
		/// \return a reference to the component of the entity
		template<typename TComponent>
		inline TComponent &Get(Entity entity) FLUFF_NOEXCEPT
		{
			static_assert((std::is_same_v<std::decay_t<TComponent>, TComponent>));
			assert(Contains(entity.Id()) && "Entity does not belong to this World");
			
			return ContainerOf(entity.Id()).template GetNext<TComponent>(entity.Id());
		}
		
		/// Gets the component of a given entity
		/// \tparam TComponent ComponentType to get
		/// \param entity that owns the wanted component
		/// \return a reference to the component of the entity
		template<typename TComponent>
		inline TComponent &Get(const Entity entity) const FLUFF_NOEXCEPT
		{
			static_assert((std::is_same_v<std::decay_t<TComponent>, TComponent>));
			assert(Contains(entity.Id()) && "Entity does not belong to this World");
			
			return ContainerOf(entity.Id()).template GetNext<TComponent>(entity.Id());
		}
		
		/// Creates an entity with the given types
		/// \tparam TComponents the entity should contain
		/// \return a new entity
		template<typename ...TComponents>
		inline Entity CreateEntity() FLUFF_NOEXCEPT
		{
			static_assert((std::is_same_v<std::decay_t<TComponents>, TComponents> && ...));
			static_assert((CanBeComponent<TComponents>() && ...));
			
			return CreateEntityImpl(internal::Sort(internal::TypeList<TComponents...>()));
		}
		
		/// Creates an entity with the given components
		/// \tparam TComponents the entity should contain
		/// \return a new entity
		template<typename ...TComponents>
		inline Entity CreateEntity(TComponents &&...args) FLUFF_NOEXCEPT
		{
			static_assert((std::is_same_v<std::decay_t<TComponents>, TComponents> && ...));
			static_assert((CanBeComponent<TComponents>() && ...));
			
			return CreateEntityImpl(internal::Sort(internal::TypeList<TComponents...>()), std::forward<TComponents>(args)...);
		}
		
		/// Creates multiple entities with the given types
		/// \tparam TComponents the entities should contain
		/// \param numEntities to create
		template<typename ...TComponents>
		inline auto CreateMultiple(EntityId numEntities) FLUFF_NOEXCEPT
		{
			static_assert((std::is_same_v<std::decay_t<TComponents>, TComponents> && ...));
			static_assert((CanBeComponent<TComponents>() && ...));
			
			_entityToContainer.Reserve(_nextFreeIndex + numEntities);
			return CreateMultipleImpl(internal::Sort(internal::TypeList<TComponents...>()), numEntities);
		}
		
		/// Creates multiple entities with the given components
		/// \tparam TComponents the entities should contain
		/// \param numEntities to create
		/// \param args components these entities shall have a copy of
		template<typename ...TComponents>
		inline auto CreateMultiple(EntityId numEntities, TComponents &&...args) FLUFF_NOEXCEPT
		{
			static_assert((CanBeComponent<TComponents>() && ...));
			
			_entityToContainer.Reserve(_nextFreeIndex + numEntities);
			return CreateMultipleImpl(internal::Sort(internal::TypeList<TComponents...>()), numEntities, std::forward<TComponents>(args)...);
		}
		
		/// Creates multiple clones from a given entity prototype
		/// \tparam TComponents the cloned entities should contain. May be less than the prototype
		template<typename ...TComponents>
		inline auto CreateMultipleFrom(EntityId numEntities, Entity prototype) FLUFF_NOEXCEPT
		{
			assert(Contains(prototype.Id()) && "Entity does not belong to a ComponentContainer");
			
			_entityToContainer.Reserve(_nextFreeIndex + numEntities);
			return CreateMultipleWith(internal::Sort(internal::TypeList<TComponents...>()), numEntities,
			                          std::forward<TComponents>(Get<TComponents>(prototype))...);
		}
		
		/// Adds one or more new components to a given entity
		/// \tparam TComponents types to add
		/// \param entity to add the component to
		template<typename ...TComponents>
		void AddComponent(Entity entity) FLUFF_NOEXCEPT
		{
			ComponentContainer &destination = AddComponentMoveImpl<TComponents...>(entity);
			(destination.GetVector<TComponents>().template PushBack<TComponents>(), ...);
		}
		
		/// Adds one or more new components to a given entity
		/// \tparam TComponents types to add (automatically deducted)
		/// \param entity to add the component to
		template<typename ...TComponents>
		void AddComponent(Entity entity, TComponents&&...comps) FLUFF_NOEXCEPT
		{
			ComponentContainer &destination = AddComponentMoveImpl<TComponents...>(entity);
			(destination.GetVector<TComponents>().template EmplaceBack<TComponents>(std::forward<TComponents>(comps)), ...);
		}
		
		template<typename TComponentToRemove>
		void RemoveComponent(Entity entity) FLUFF_NOEXCEPT
		{
			assert(Contains(entity.Id()) && "Entity does not belong to this World");
			ComponentContainer &source = ContainerOf(entity.Id());
			
			const auto &sourceTInfo = source.GetContainedTypes();
			std::pmr::vector<IdType> targetIds{&_tempResource};
			targetIds.reserve(sourceTInfo.size() - 1);
			
			for(auto i : sourceTInfo)
			{
				// don't add the type id we actually want to remove
				if(i.id != TypeId<TComponentToRemove>)
					targetIds.push_back(i.id);
			}
			
			
			const auto combinedTargetIds = internal::CombineIds(targetIds.cbegin(), targetIds.cend());
			if(_componentContainers.count(combinedTargetIds))
			{
				ComponentContainer &destination = *_componentContainers.at(combinedTargetIds);
				source.MoveEntityTo(destination, entity.Id());
			} else
			{
				auto &sourceConstructors = source.GetConstructorTable();
				
				// create list of all type information
				std::pmr::vector<TypeInformation> targetTypes{&_tempResource};
				std::pmr::vector<internal::ConstructorVTable> targetConstructors{&_tempResource};
				targetTypes.reserve(targetIds.size());
				targetConstructors.reserve(targetIds.size());
				for(std::size_t i = 0; i < targetIds.size(); ++i)
				{
					if(sourceTInfo[i].id != TypeId<TComponentToRemove>)
					{
						targetTypes.push_back(sourceTInfo[i]);
						targetConstructors.push_back(sourceConstructors[i]);
					}
				}
				
				auto &destination = CreateComponentContainerWith(targetTypes, targetConstructors);
				source.MoveEntityTo(destination, entity.Id());
			}
		}
	
	public:
		/// Collects all Entities that have at least the given components
		/// \tparam TComponents that collected entities need to contain at minimum
		/// \return a list of iterators that go through all containers containing the given components
		template<typename ...TComponents>
		MultiContainerRange<TComponents...> EntitiesWith() FLUFF_NOEXCEPT
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			return MultiContainerRange<TComponents...>(std::forward<std::vector<ComponentContainer *>>(CollectVectorsOf<TComponents...>()));
		}
		
		/// Collects all Entities that have at least the given components
		/// \tparam TComponents that collected entities need to contain at minimum
		/// \return a list of iterators that go through all containers containing the given components
		template<typename ...TComponents>
		MultiContainerRange<const TComponents...> EntitiesWith() const FLUFF_NOEXCEPT
		{
			static_assert(not((std::is_reference_v<TComponents> || std::is_pointer_v<TComponents>) || ...), "Types cannot be reference or pointer");
			
			return MultiContainerRange<const TComponents...>(std::forward<std::vector<ComponentContainer *>>
					                                                 (const_cast<BasicWorld *>(this)->CollectVectorsOf<TComponents...>()));
		}
	
	public:
		/// Checks whether a given type can be used as a component for this ECS. It needs to be default
		/// constructible, copy constructible and move constructible
		/// \tparam TComponent type to check
		/// \return whether this is usable as a component type
		template<typename TComponent>
		static constexpr bool CanBeComponent() noexcept
		{
			return std::is_default_constructible_v<TComponent> &&
			       (std::is_copy_constructible_v<TComponent> && std::is_move_constructible_v<TComponent>);
		}
	
	private:
		template<typename TAllocator1, typename TAllocator2>
		ComponentContainer &CreateComponentContainerWith(const std::vector<TypeInformation, TAllocator1> &infos,
		                                                 const std::vector<internal::ConstructorVTable, TAllocator2> &constructors) FLUFF_NOEXCEPT
		{
			auto *createdContainer = (ComponentContainer *)
					_containerResource.allocate(sizeof(ComponentContainer), alignof(ComponentContainer));
			
			// place new ComponentVector into that location
			createdContainer = new(createdContainer) ComponentContainer(_containerResource);
			std::pmr::vector<IdType> ids{infos.size(), &_tempResource};
			for(std::size_t i = 0; i < infos.size(); ++i)
			{
				createdContainer->AddVector(infos[i], constructors[i], GetMemoryResource(infos[i].id));
				ids[i] = infos[i].id;
			}
			
			RegisterVector(createdContainer, createdContainer->GetMultiTypeId(), ids);
			return *createdContainer;
		}
		
		/// Collects all vectors of ComponentVectors that contain at least the given types
		/// \tparam TComponents the types that the ComponentVectors need to contain at least to be relevant
		/// \return a list of tuples of vectors containing the given components
		template<typename ...TComponents>
		std::vector<ComponentContainer *> CollectVectorsOf() noexcept
		{
			return CollectVectorsOfImpl(internal::Sort(internal::TypeList<std::remove_const_t<std::remove_reference_t<TComponents>>...>()));
		}
		
		/// Collects all vectors of ComponentVectors that contain at least the given types
		/// \tparam TComponents the types that the ComponentVectors need to contain at least to be relevant
		/// \return a list of tuples of vectors containing the given components
		template<typename ...TComponents>
		std::vector<const ComponentContainer *> CollectVectorsOf() const noexcept
		{
			return CollectVectorsOfImpl(internal::Sort(internal::TypeList<std::remove_const_t<std::remove_reference_t<TComponents>>...>()));
		}
		
		/// Collects all vectors containing at minimum the given components
		/// \return a list of pointers to those containers
		template<typename ...TComponents>
		std::vector<ComponentContainer *> CollectVectorsOfImpl(internal::TypeList<TComponents...>) noexcept
		{
			return _vectorsMap.GetAllFromSequence<sizeof...(TComponents)>({TypeId<TComponents> ...});
		}
		
		/// Collects all vectors containing at minimum the given components
		/// \return a list of const pointers to those containers
		template<typename ...TComponents>
		std::vector<const ComponentContainer *> CollectVectorsOfImpl(internal::TypeList<TComponents...>) const noexcept
		{
			return _vectorsMap.GetAllFromSequence<sizeof...(TComponents)>({TypeId<TComponents> ...});
		}
		
		template<typename ...TComponents>
		inline Entity CreateEntityImpl(internal::TypeList<TComponents...>) FLUFF_NOEXCEPT
		{
			ComponentContainer &vec = GetComponentVector<TComponents...>();
			return Entity(vec.PushBack<TComponents...>(), *static_cast<internal::WorldInternal *>(this));
		}
		
		template<typename ...TComponents, typename ...TAddedComponents>
		inline Entity CreateEntityImpl(internal::TypeList<TComponents...>, TAddedComponents &&...args) FLUFF_NOEXCEPT
		{
			ComponentContainer &vec = GetComponentVector<TComponents...>();
			return Entity(vec.EmplaceBack(std::forward<TAddedComponents>(args)...), *static_cast<internal::WorldInternal *>(this));
		}
		
		template<typename ...TComponents>
		inline Iterator<TComponents...> CreateMultipleImpl(internal::TypeList<TComponents...>, EntityId numEntities) FLUFF_NOEXCEPT
		{
			ComponentContainer &vec = GetComponentVector<TComponents...>();
			return vec.CreateMultiple<TComponents...>(numEntities);
		}
		
		template<typename ...TComponents, typename ...TComponentsToAdd>
		inline Iterator<TComponents...> CreateMultipleWith(internal::TypeList<TComponents...>, EntityId numEntities, TComponentsToAdd &&...args)
		{
			ComponentContainer &vec = GetComponentVector<TComponents...>();
			return vec.Clone(numEntities, std::forward<TComponentsToAdd>(args)...);
		}
		
		/// Moves the entity to archetype wit entities components + TAddedComponents, but does not create TAddedComponents!
		/// \tparam TAddedComponents
		/// \param entity to gain new components
		/// \return the vector where the entity now resides in
		template<typename ...TAddedComponents>
		ComponentContainer &AddComponentMoveImpl(Entity entity)
		{
			static_assert(sizeof...(TAddedComponents) != 0);
			static_assert((CanBeComponent<TAddedComponents>() && ...));
			
			assert(Contains(entity.Id())); // Entity does not belong to this World
			ComponentContainer &source = ContainerOf(entity.Id());
			
			std::array<IdType, sizeof...(TAddedComponents)> tIdsToAdd = {TypeId<TAddedComponents> ...};
			const MultiIdType destinationTypeId = internal::CombineIds(source.GetIds().cbegin(), source.GetIds().cend(), tIdsToAdd.cbegin(), tIdsToAdd.cend());
			
			ComponentContainer *destination{};
			if(_componentContainers.count(destinationTypeId))
			{
				destination= _componentContainers.at(destinationTypeId);
			} else
			{
				// need to create a new container for this entity
				std::pmr::vector<TypeInformation> tInfos{source.GetTypeInfos(), &_tempResource};
				std::pmr::vector<internal::ConstructorVTable> constructors{source.GetConstructorTable(), &_tempResource};
				
				// the vector is already sorted, we only need to insert the new data at the correct position to keep it sorted
				std::array<TypeInformation, sizeof...(TAddedComponents)> tInfosToAdd = {TypeInformation::Of<TAddedComponents>() ...};
				std::array<internal::ConstructorVTable, sizeof...(TAddedComponents)> constructorsToAdd = {internal::ConstructorVTable::Of<TAddedComponents>() ...};
				for(std::size_t i = 0; i < tIdsToAdd.size(); ++i)
				{
					std::size_t insertionPosition = 0;
					for(; insertionPosition < tInfos.size(); ++insertionPosition)
					{
						if(tInfosToAdd[i].id < tInfos[insertionPosition].id)
							break;
					}
					
					// add info here
					tInfos.insert(tInfos.cbegin() + insertionPosition, tInfosToAdd[i]);
					constructors.insert(constructors.cbegin() + insertionPosition, constructorsToAdd[i]);
				}
				
				destination = &CreateComponentContainerWith(tInfos, constructors);
			}
			
			source.MoveEntityTo(*destination, entity.Id());
			return *destination;
		}
		
		/// Looks up the component container containing EXACTLY the given components, or, if none is found, creates a new one
		/// \tparam TComponents the container contains
		/// \return a reference to that container
		template<typename ...TComponents>
		ComponentContainer &GetComponentVector()
		{
			constexpr MultiIdType id = MultiTypeId<TComponents...>;
			
			if(_componentContainers.count(id) != 0)
			{
				return *_componentContainers.at(id);
			} else
			{
				// allocate memory for that vector
				auto *createdVector = (ComponentContainer *)
						_containerResource.allocate(sizeof(ComponentContainer), alignof(ComponentContainer));
				
				// place new ComponentVector into that location
				createdVector = new(createdVector) ComponentContainer(_containerResource);
				((createdVector->AddVector<TComponents>(GetMemoryResource(TypeId<TComponents>))), ...);
				
				
				RegisterVector(createdVector, id, std::pmr::vector<IdType>({TypeId<TComponents>...}, &_tempResource));
				return *createdVector;
			}
		}
		
		/// Registers the given container in this world
		/// \param container to register
		/// \param multiId of types contained in it
		/// \param individualIds of the types in the container
		template<typename TAllocator>
		void RegisterVector(ComponentContainer *container, MultiIdType multiId,
		                    const std::vector<IdType, TAllocator> &individualIds) FLUFF_NOEXCEPT
		{
			container->world = static_cast<internal::WorldInternal *>(this);
			_componentContainers.insert({multiId, container});
			_vectorsMap.Insert(individualIds, container);
		}
	
	private:
		/// Looks up the memory resource for a given type, or, if none is found, creates a new one
		/// \param id TypeId of the type of the resource
		/// \return a reference to that memory resource
		TMemResource &GetMemoryResource(IdType id) FLUFF_NOEXCEPT
		{
			if(_resources.count(id) != 0)
			{
				return *_resources.at(id);
			} else
			{
				// create new memory resource
				if constexpr (std::is_constructible_v<TMemResource, std::pmr::pool_options>)
				{
					return *_resources.insert({id, std::make_unique<TMemResource>(STANDARD_POOL_OPTIONS)}).first->second;
				} else
				{
					return *_resources.insert({id, std::make_unique<TMemResource>()}).first->second;
				}
			}
		}
	
	private:
		/// Used to handle component vector allocations (and deallocations)
		TMemResource _containerResource{};
		
		/// maps a type id to a memory resource containing that type
		Map<IdType, std::unique_ptr<TMemResource>> _resources{};
		/// maps multi type id to that specific component vector
		Map<MultiIdType, ComponentContainer *> _componentContainers{};
		/// maps a sequence of component types to component containers that contain those
		internal::SortedKeyTree<IdType, ComponentContainer *> _vectorsMap{_tempResource};
	};
	
	using World = BasicWorld<std::pmr::unsynchronized_pool_resource>;
	
	template<typename TComponent>
	TComponent *Entity::Get() noexcept
	{
		if(not _world) FLUFF_UNLIKELY
			return nullptr;
		
		ComponentContainer &cont = _world->ContainerOf(Id());
		if(cont.ContainsId(Id()))
		{
			return &cont.Get<TComponent>(Id());
		} else
		{
			return nullptr;
		}
	}
	
	template<typename TComponent>
	const TComponent *Entity::Get() const noexcept
	{
		if(not _world) FLUFF_UNLIKELY
			return nullptr;
		
		ComponentContainer &cont = _world->ContainerOf(Id());
		if(cont.ContainsId(Id()))
		{
			return &cont.Get<TComponent>(Id());
		} else
		{
			return nullptr;
		}
	}
	
	void Entity::Destroy() FLUFF_NOEXCEPT
	{
		ComponentContainer &cont = _world->ContainerOf(Id());
		cont.Remove(Id());
	}
	
	template<typename TComponent>
	bool Entity::Has() const noexcept
	{
		if(not _world) FLUFF_UNLIKELY
			return false;
		
		const ComponentContainer &cont = _world->ContainerOf(Id());
		return cont.ContainsId(Id());
	}
}
#endif //FLUFF_ECS_WORLD_H
