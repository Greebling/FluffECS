#pragma once

#include <cassert>
#include <vector>
#include <memory_resource>

#include "TypeId.h"
#include "Entity.h"
#include "TypeList.h"
#include "DynamicVector.h"
#include "SparseSet.h"
#include "VirtualConstructor.h"
#include "WorldInternal.h"

namespace flf
{
	template<typename ...TComponents>
	class IteratorContainer;
	
	class ComponentContainer
	{
	public:
		template<typename T>
		using VectorOf = std::pmr::vector<T>;
		using IndexType = EntityId;
		
		/// the number of elements that will be reserved at the construction of the ComponentVector. Does not need to
		/// be large due to increased efficiency of pmr memory resources
		static constexpr IndexType VECTOR_PRE_RESERVE_AMOUNT = 32;
	
	public:
		explicit ComponentContainer(std::pmr::memory_resource &resource) FLUFF_NOEXCEPT
				: _ownResource(&resource)
		{
		}
		
		ComponentContainer() FLUFF_NOEXCEPT = default;
		
		ComponentContainer(const ComponentContainer &other) FLUFF_MAYBE_NOEXCEPT
				: _ownResource(other._ownResource),
				  _sparse(other._sparse),
				  _typeInfos(other._typeInfos),
				  _componentVectors(other._ownResource)
		{
		}
		
		ComponentContainer(ComponentContainer &&other) FLUFF_MAYBE_NOEXCEPT
				: _ownResource(other._ownResource),
				  _sparse(std::move(other._sparse)),
				  _typeInfos(std::move(other._typeInfos)),
				  _componentVectors(other._ownResource)
		{
		}
	
	public:
		/*
		 * Individual component methods
		 */
		
		/// Gets the Component part of an entity
		/// \tparam TWantedComponent the wanted component
		/// \param entity the owning entity
		/// \return a reference to that component
		template<typename TComponent>
		[[nodiscard]] inline TComponent &Get(EntityId entity) FLUFF_NOEXCEPT
		{
#ifdef FLUFF_DO_RANGE_CHECKS
			assert(Contains<TComponent>(entity));
#endif
			const auto index = IndexOf(entity);
			return GetVector<TComponent>().template Get<TComponent>(index);
		}
		
		/// Gets the Component part of an entity
		/// \tparam TWantedComponent the wanted component
		/// \param entity the owning entity
		/// \return a reference to that component
		template<typename TComponent>
		[[nodiscard]] inline const TComponent &Get(EntityId entity) const FLUFF_NOEXCEPT
		{
#ifdef FLUFF_DO_RANGE_CHECKS
			assert(Contains<TComponent>(entity));
#endif
			return GetVector<TComponent>().template Get<TComponent>(IndexOf(entity));
		}
		
		/// \param entity
		/// \return the index in this container that the entity is assigned to
		[[nodiscard]] inline IndexType IndexOf(EntityId entity) const FLUFF_MAYBE_NOEXCEPT
		{
			return _sparse[entity];
		}
		
		/// Creates a single entity with the given components
		/// \tparam TComponents of the entity
		/// \return the id of the created entity
		template<typename ...TComponents>
		EntityId PushBack() FLUFF_MAYBE_NOEXCEPT
		{
			assert("Given Component types were not in container!" && sizeof...(TComponents) == _typeInfos.size());
			
			((GetVector<TComponents>().template PushBack<TComponents>()), ...);
			auto index = world->TakeNextFreeIndex(*this);
			_sparse.AddEntry(index, _componentIds.size());
			_componentIds.push_back(index);
			return index;
		}
		
		/// Creates a single entity with the given components
		/// \tparam TComponents of the entity
		/// \return the id of the created entity
		template<typename ...TComponents>
		EntityId EmplaceBack(TComponents &&...components) FLUFF_MAYBE_NOEXCEPT
		{
			assert(sizeof...(TComponents) == _typeInfos.size() && "Given Component types were not in container!");
			
			((GetVector<TComponents>().template EmplaceBack<TComponents>(std::forward<TComponents>(components))), ...);
			auto index = world->TakeNextFreeIndex(*this);
			_sparse.AddEntry(index, _componentIds.size());
			_componentIds.push_back(index);
			return index;
		}
		
		/// Creates a multiple entities with the given components
		/// \tparam TComponents of the entity
		template<typename ...TComponents>
		void CreateMultiple(const EntityId number) FLUFF_MAYBE_NOEXCEPT
		{
			assert(sizeof...(TComponents) == _typeInfos.size() && "Given Component types were not in container!");
			
			const auto beginSize = _componentIds.size();
			const auto endSize = beginSize + number;
			
			RegisterMultiple(beginSize, endSize);
			// create component data
			((GetVector<TComponents>().template Resize<TComponents>(beginSize + number)), ...);
		}
		
		/// Creates a multiple entities with the given components
		/// \tparam TComponents of the container
		/// \param amount of clones to create
		/// \param id of the entity to clone
		template<typename ...TComponents>
		inline void Clone(const IndexType amount, const EntityId id) FLUFF_MAYBE_NOEXCEPT
		{
			assert(ContainsId(id) && "EntityId not found in this container");
			assert((ContainsType(TypeId<TComponents>()) && ...) && "Component types are not in this vector");
			
			const auto beginSize = _componentIds.size();
			const auto endSize = beginSize + amount;
			const auto index = IndexOf(id);
			
			RegisterMultiple(beginSize, endSize);
			Clone<TComponents...>(amount, Get<TComponents>(index)...);
		}
		
		/// Creates a multiple entities with the given components
		/// \tparam TComponents of the container
		/// \param amount of clones to create
		/// \param components to clone from
		template<typename ...TComponents>
		inline void Clone(const IndexType amount, const TComponents &...components) FLUFF_MAYBE_NOEXCEPT
		{
			assert((ContainsType(TypeId<TComponents>()) && ...) && "Component types are not in this vector");
			const auto beginSize = _componentIds.size();
			const auto endSize = beginSize + amount;
			
			RegisterMultiple(beginSize, endSize);
			((GetVector<TComponents>().template Clone<TComponents>(amount, components)), ...);
		}
		
		/// Removes all components associated with the given id
		/// \param id of the entity to remove
		void Remove(EntityId id) FLUFF_NOEXCEPT
		{
			assert(ContainsId(id) && "Entity was not in Container");
			
			if (!ContainsId(id))
			{
				// we have nothing to delete
				return;
			}
			
			
			if (const auto index = IndexOf(id); index != Size())
			{
				const EntityId movedEntity = _componentIds.back();
				_componentIds[index] = movedEntity;
				_sparse.SetEntry(movedEntity, index);
				
				// move components from back to index as we don't need the data at index anymore
				for (IndexType i = 0; i < _typeInfos.size(); ++i)
				{
					const auto tInfo = _typeInfos[i];
					auto &currVector = _componentVectors[i];
					
					const auto bytePosition = tInfo.size * index;
					void *dataToMove = currVector.GetBytes(bytePosition);
					void *end = currVector.BackPtr() - tInfo.size;
					
					if (dataToMove != end) FLUFF_LIKELY
					{
						_constructors[i].moveConstruct(dataToMove, end);
					}
					_constructors[i].destruct(dataToMove);
				}
			}
			
			// mark entity as deleted in sparse map
			_sparse.MarkAsDeleted(id);
			
			// Pop back vectors; data at end may be removed
			_componentIds.pop_back();
			for (IndexType i = 0; i < _typeInfos.size(); ++i)
			{
				_componentVectors[i].PopBackBytes(_typeInfos[i].size);
			}
		}
		
		/// Reserves the given amount of components
		/// \tparam TComponents to reserve
		/// \param n amount of entries to reserve
		template<typename ...TComponents>
		void Reserve(IndexType n)
		{
			((GetVector<TComponents>().template Reserve<TComponents>(n)), ...);
		}
		
		/// \return the list of saved entity ids. the list is in the same order as the component data
		[[nodiscard]] inline const VectorOf<EntityId> &GetIds() const FLUFF_NOEXCEPT
		{
			return _componentIds;
		}
		
		/// \return the number of entities contained
		[[nodiscard]] inline IndexType Size() const FLUFF_NOEXCEPT
		{
			return _componentIds.size();
		}
		
		/// \return the capacity of the underlying vectors
		[[nodiscard]] inline IndexType Capacity() const FLUFF_NOEXCEPT
		{
			if (_componentVectors.empty())
			{
				return _componentIds.capacity();
			} else
			{
				return _componentVectors[0].ByteCapacity() / _typeInfos[0].size;
			}
		}
	
	public:
		/// \return the MultiIdType of the sum of all contained components
		[[nodiscard]] inline MultiIdType GetMultiTypeId() const FLUFF_NOEXCEPT
		{
			MultiIdType result = MultiTypeId<>();
			for (auto tInfo : _typeInfos)
			{
				result = result xor tInfo.id;
			}
			return result;
		}
	
	public:
		/*
		 * Dynamic Vector Methods
		 */
		
		/// Adds a vector for the given component type
		/// \tparam TComponent to be containable
		/// \param resource to use for that vector
		/// \return a reference to the newly created vector
		template<typename TComponent>
		inline internal::DynamicVector &AddVector(std::pmr::memory_resource &resource) FLUFF_MAYBE_NOEXCEPT
		{
			assert("Type already in container!" && !ContainsType(TypeId<TComponent>()));
			
			internal::DynamicVector &createdVector =
					AddVector(TypeInformation::Of<TComponent>(), internal::ConstructorVTable::Of<TComponent>(), resource);
			createdVector.Reserve<TComponent>(VECTOR_PRE_RESERVE_AMOUNT);
			return createdVector;
		}
		
		/// Adds a vector for the given type
		/// \param type to contain
		/// \param resource to use for the vector
		/// \return a pointer to the created vector
		internal::DynamicVector &
		AddVector(TypeInformation type, internal::ConstructorVTable constructors, std::pmr::memory_resource &resource) FLUFF_MAYBE_NOEXCEPT
		{
			assert(!ContainsType(type.id) && "Type already in container!");
			
			_typeInfos.push_back(type);
			_constructors.emplace_back(constructors);
			
			return _componentVectors.emplace_back(resource);
		}
		
		/// \tparam TComponent type to contain in the vector
		/// \return a reference to a vector containing that type
		template<typename TComponent>
		[[nodiscard]] inline const internal::DynamicVector &GetVector() const FLUFF_NOEXCEPT
		{
			assert(ContainsType(TypeId<TComponent>()) && "Type not in ComponentContainer");
			return *GetVector(TypeId<TComponent>());
		}
		
		/// \tparam TComponent type to contain in the vector
		/// \return a reference to a vector containing that type
		template<typename TComponent>
		[[nodiscard]] inline internal::DynamicVector &GetVector() FLUFF_NOEXCEPT
		{
			assert(ContainsType(TypeId<TComponent>()) && "Type not in ComponentContainer");
			return *GetVector(TypeId<TComponent>());
		}
		
		/// Moves all data associated with the given entity to another ComponentContainer
		/// \param destination to move the data to
		/// \param id associated with the data to be moved
		void MoveEntityTo(ComponentContainer &destination, EntityId id) FLUFF_MAYBE_NOEXCEPT
		{
			assert(ContainsId(id) && "Id not contained!");
			
			const auto index = IndexOf(id);
			
			for (std::size_t i = 0; i < _typeInfos.size(); ++i)
			{
				const TypeInformation tInfo = _typeInfos[i];
				
				internal::DynamicVector *targetVector = destination.GetVector(tInfo.id);
				internal::DynamicVector *ownByteData = GetVector(tInfo.id);
				const auto bytePosition = tInfo.size * index;
				
				
				if (targetVector)
				{
					// copy data to target
					if (ownByteData)
					{
						void *dataToMove = ownByteData->GetBytes(bytePosition);
						
						targetVector->EmplaceBackUsing(dataToMove, tInfo.size, _constructors[i]);
					} else
					{
						targetVector->PushBackUsing(tInfo.size, _constructors[i]);
					}
				}
			}
			
			// Register new entity
			world->AssociateIdWith(id, destination);
			destination._sparse.AddEntry(id, destination._componentIds.size());
			destination._componentIds.push_back(id);
			
			Remove(id);
		}
		
		/// Reserves a given amount of different component types
		inline void ReserveComponentTypes(std::size_t amount) FLUFF_MAYBE_NOEXCEPT
		{
			_typeInfos.reserve(amount);
			_componentVectors.reserve(amount);
		}
		
		/// \tparam TComponent type to check for
		/// \param id of the entity to be checked
		/// \return
		template<typename TComponent>
		[[nodiscard]] inline bool Contains(EntityId id) const FLUFF_NOEXCEPT
		{
			return ContainsType(TypeId<TComponent>()) && ContainsId(id);
		}
		
		/// Checks whether a given id is in this container
		/// \param id to check for
		/// \return true when the id is in this container
		[[nodiscard]] inline bool ContainsId(EntityId id) const FLUFF_NOEXCEPT
		{
			return _sparse.Contains(id);
		}
		
		/// \param type to check for
		/// \return true if the type has a corresponding dynamic vector in this container
		[[nodiscard]] inline bool ContainsType(IdType type) const FLUFF_NOEXCEPT
		{
			for (const auto tInfo : _typeInfos)
			{
				if (tInfo.id == type)
				{
					return true;
				}
			}
			return false;
		}
		
		/// \return A vector with TypeInformation to all types in this container
		[[nodiscard]] inline std::vector<TypeInformation> GetContainedTypes() const FLUFF_MAYBE_NOEXCEPT
		{
			return std::vector<TypeInformation>(_typeInfos.cbegin(), _typeInfos.cend());
		}
		
		
		/// \return a list of all DynamicVectors that are contained in this
		[[nodiscard]] inline const VectorOf<internal::DynamicVector> &GetAllVectors() const FLUFF_NOEXCEPT
		{
			return _componentVectors;
		}
		
		/// Sets the used memory resource of the vector to a different one.
		/// WARNING: May only be used when this container does not contain anything
		/// \param resource to use for this container
		void SetOwnMemoryResource(std::pmr::memory_resource &resource) FLUFF_NOEXCEPT
		{
			assert((_ownResource == nullptr || _typeInfos.empty()) &&
			       "Cannot reset memory resource as the previous one was already used");
			
			_ownResource = &resource;
			_typeInfos = VectorOf<TypeInformation>(_ownResource);
			_constructors = VectorOf<internal::ConstructorVTable>(_ownResource);
			_componentVectors = VectorOf<internal::DynamicVector>(_ownResource);
		}
		
		[[nodiscard]] const VectorOf<TypeInformation> &GetTypeInfos() const FLUFF_NOEXCEPT
		{
			return _typeInfos;
		}
		
		[[nodiscard]] const VectorOf<internal::ConstructorVTable> &GetConstructorTable() const FLUFF_NOEXCEPT
		{
			return _constructors;
		}
	
	private:
		/// Register multiple entities at once
		/// \param beginSize size of the container BEFORE the creation of these entities
		/// \param endSize size of the container AFTER the creation of these entities
		void RegisterMultiple(IndexType beginSize, IndexType endSize) FLUFF_MAYBE_NOEXCEPT
		{
			_componentIds.resize(endSize);
			_sparse.Resize(endSize);
			
			for (IndexType i = beginSize; i < endSize; ++i)
			{
				auto index = world->TakeNextFreeIndex(*this);
				_sparse.SetEntry(index, i);
				_componentIds[i] = index;
			}
		}
		
		/// \return a list of all DynamicVectors that are contained in this
		[[nodiscard]] inline VectorOf<internal::DynamicVector> &GetAllVectors() FLUFF_NOEXCEPT
		{
			return _componentVectors;
		}
		
		/// Gets the vector that contains the type with the given TypeId
		/// \param type to be contained in that vector
		/// \return A pointer to the vector or nullptr if there is no vector containing that type
		[[nodiscard]] inline internal::DynamicVector *GetVector(IdType type) FLUFF_NOEXCEPT
		{
			for (std::size_t i = 0; i < _typeInfos.size(); ++i)
			{
				if (_typeInfos[i].id == type)
				{
					return &_componentVectors[i];
				}
			}
			return nullptr;
		}
		
		/// Gets the vector that contains the type with the given TypeId
		/// \param type to be contained in that vector
		/// \return A pointer to the vector or nullptr if there is no vector containing that type
		[[nodiscard]] inline const internal::DynamicVector *GetVector(IdType type) const FLUFF_NOEXCEPT
		{
			for (std::size_t i = 0; i < _typeInfos.size(); ++i)
			{
				if (_typeInfos[i].id == type)
				{
					return &_componentVectors[i];
				}
			}
			return nullptr;
		}
	
	private:
		/// Memory where the actual pages of the sparse map will be saved to
		std::pmr::unsynchronized_pool_resource _sparseMemory{{4, 4096}};
	
	public:
		/// Point to the owning world, where this container is located. Useful for getting the next free EntityId
		internal::WorldInternal *world{};
	
	public:
		/// \tparam TComponents to get. Need to be contained in this ComponentContainer!
		/// \return a tuple of pointers to the begin of all given components
		template<typename ...TComponents>
		std::tuple<TComponents *...> RawBegin() FLUFF_NOEXCEPT
		{
			std::array<void *, sizeof...(TComponents)> pointers{};
			
			{
				constexpr std::array<IdType, sizeof...(TComponents)> ids = SortedTypeIdList<std::remove_cvref_t<TComponents>...>();
				
				std::size_t currPointersIndex = 0;
				std::size_t i = 0;
				while (currPointersIndex < pointers.size())
				{
					assert(i < _typeInfos.size());// need to have found all pointers in container!
					
					if (_typeInfos[i].id == ids[currPointersIndex])
					{
						pointers[currPointersIndex] = _componentVectors[i].Data();
						++currPointersIndex;
						i = 0; // i will be incremented after this
					} else
					{
						++i;
					}
				}
			}
			return PointerArrayToTuple<std::remove_reference_t<TComponents>...>(pointers,
			                                                                    std::make_index_sequence<sizeof...(TComponents)>());
		}
		
		/// \tparam TComponents to get. Need to be contained in this ComponentContainer!
		/// \return a tuple of pointers to the end to of all given components
		template<typename ...TComponents>
		std::tuple<TComponents *...> RawEnd() FLUFF_NOEXCEPT
		{
			assert(ContainsType(TypeId<std::remove_cvref_t<TComponents>>()) && ...);
			
			std::array<void *, sizeof...(TComponents)> pointers{};
			
			{
				constexpr std::array<IdType, sizeof...(TComponents)> ids = SortedTypeIdList<std::remove_cvref_t<TComponents>...>();
				
				std::size_t currPointersIndex = 0;
				std::size_t i = 0;
				while (currPointersIndex < pointers.size())
				{
					assert(i < _typeInfos.size());// need to have found all pointers in container!
					
					if (_typeInfos[i].id == ids[currPointersIndex])
					{
						pointers[currPointersIndex] = _componentVectors[i].End();
						++currPointersIndex;
						i = 0; // i will be incremented after this
					} else
					{
						++i;
					}
					
				}
			}
			
			return PointerArrayToTuple<std::remove_reference_t<TComponents>...>(pointers,
			                                                                    std::make_index_sequence<sizeof...(TComponents)>());
		}
		
		/// \tparam TComponents to get. Need to be contained in this ComponentContainer!
		/// \return a tuple of pointers to the begin to of all given components and to the EntityIds
		template<typename ...TComponents>
		std::tuple<TComponents *..., EntityId *> RawBeginWithEntity() FLUFF_NOEXCEPT
		{
			std::array<void *, sizeof...(TComponents)> pointers{};
			
			{
				constexpr std::array<IdType, sizeof...(TComponents)> ids = SortedTypeIdList<std::remove_cvref_t<TComponents>...>();
				
				std::size_t currPointersIndex = 0;
				std::size_t i = 0;
				while (currPointersIndex < pointers.size())
				{
					assert(i < _typeInfos.size());// need to have found all pointers in container!
					
					if (_typeInfos[i].id == ids[currPointersIndex])
					{
						pointers[currPointersIndex] = _componentVectors[i].Data();
						++currPointersIndex;
						i = 0; // i will be incremented after this
					} else
					{
						++i;
					}
				}
			}
			
			auto componentData = PointerArrayToTuple<std::remove_reference_t<TComponents>...>
					(pointers, std::make_index_sequence<sizeof...(TComponents)>());
			return std::tuple_cat(componentData, std::make_tuple(_componentIds.data()));
		}
		
		/// \tparam TComponents to get. Need to be contained in this ComponentContainer!
		/// \return a tuple of pointers to the end to of all given components and to the EntityIds
		template<typename ...TComponents>
		std::tuple<TComponents *..., EntityId *> RawEndWithEntity() FLUFF_NOEXCEPT
		{
			std::array<void *, sizeof...(TComponents)> pointers{};
			
			{
				constexpr std::array<IdType, sizeof...(TComponents)> ids = SortedTypeIdList<std::remove_cvref_t<TComponents>...>();
				
				std::size_t currPointersIndex = 0;
				std::size_t i = 0;
				while (currPointersIndex < pointers.size())
				{
					assert(i < _typeInfos.size());// need to have found all pointers in container!
					
					if (_typeInfos[i].id == ids[currPointersIndex])
					{
						pointers[currPointersIndex] = _componentVectors[i].End();
						++currPointersIndex;
						i = 0; // i will be incremented after this
					} else
					{
						++i;
					}
				}
			}
			
			auto componentData = PointerArrayToTuple<std::remove_reference_t<TComponents>...>
					(pointers, std::make_index_sequence<sizeof...(TComponents)>());
			return std::tuple_cat(componentData, std::make_tuple(_componentIds.data() + _componentIds.size()));
		}
	
	private:
		/// Casts a collection of void pointers to typed pointers of a given type
		/// \tparam TComponents types
		/// \tparam Is (deduced from make_index_sequence)
		/// \param pointers to cast
		/// \param seq generated from std::make_index_sequence<sizeof...(TComponents)>()
		/// \return a tuple of correctly typed pointers
		template<typename ...TComponents, std::size_t ...Is>
		static constexpr std::tuple<TComponents *...>
		PointerArrayToTuple(std::array<void *, sizeof...(TComponents)> pointers,
		                    std::integer_sequence<std::size_t, Is...> seq) FLUFF_NOEXCEPT
		{
			static_assert(sizeof...(TComponents) == sizeof...(Is));
			return {reinterpret_cast<TComponents *>(pointers[Is])...};
		}
	
	private:
		/// The resource the vectors own data is saved in
		std::pmr::memory_resource *_ownResource = nullptr;
		
		/// Saves the EntityId of the entity at the same position in components
		VectorOf<EntityId> _componentIds{&_sparseMemory};
		
		/// The indices to the dense array, using entity ids as key. Maps from EntityId to the index into _components
		internal::SparseSet<EntityId, IndexType> _sparse{_sparseMemory};
		
		/// Info to the types saved in this container
		VectorOf<TypeInformation> _typeInfos{_ownResource};
		
		/// Contains VTables for constructing a given type
		VectorOf<internal::ConstructorVTable> _constructors{_ownResource};
		
		/// Contains vectors of the components
		VectorOf<internal::DynamicVector> _componentVectors{_ownResource};
	};
	
	
}
