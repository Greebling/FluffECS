#pragma once

#include <memory_resource>
#include <utility>
#include "Keywords.h"
#include "SparseSet.h"
#include "Entity.h"

namespace flf
{
	class Archetype;
}

namespace flf::internal
{
	class WorldInternal
	{
	public:
		[[nodiscard]] bool Contains(EntityId id) const FLUFF_NOEXCEPT
		{
			return _entityToContainer.Contains(id);
		}
		
		[[nodiscard]] Archetype &ContainerOf(EntityId id) FLUFF_NOEXCEPT
		{
			return *_entityToContainer[id];
		}
		
		[[nodiscard]] const Archetype &ContainerOf(EntityId id) const FLUFF_NOEXCEPT
		{
			return *_entityToContainer[id];
		}
		
		inline std::pair<EntityId, EntityId> GetNextIndicesRange(EntityId n, Archetype &owner)
		{
			// TODO: This causes bad alloc for large sizes, but the single one does not. why?
			auto beginIndex = _nextFreeIndex;
			auto endIndex = _nextFreeIndex + n;
			_nextFreeIndex = endIndex + 1;
			
			_entityToContainer.AddRange(beginIndex, endIndex, &owner);
			
			return {beginIndex, endIndex};
		}
		
		/// Creates a new unique id for an entity
		/// \param owner of that new entity
		/// \return the a free unique id for an entity in this world
		[[nodiscard]] inline EntityId PeekNextFreeIndex() const FLUFF_MAYBE_NOEXCEPT
		{
			return _nextFreeIndex;
		}
		
		/// Creates a new unique id for an entity
		/// \param owner of that new entity
		/// \return the a free unique id for an entity in this world
		[[nodiscard]] inline EntityId TakeNextFreeIndex(Archetype &owner) FLUFF_MAYBE_NOEXCEPT
		{
			auto index = _nextFreeIndex;
			// TODO: This is a performance bottleneck. Can we make this more efficient than taking much of the time of creating many entities?
			_entityToContainer.AddEntry(index, &owner);
			
			++_nextFreeIndex;
			return index;
		}
		
		inline void AssociateIdWith(EntityId id, Archetype &container) FLUFF_NOEXCEPT
		{
			_entityToContainer.SetEntry(id, &container);
		}
	
	protected:
		EntityId _nextFreeIndex = 0;
		
		std::pmr::monotonic_buffer_resource _sparseMemory{8192};
		/// Used for small, temporary allocations
		std::pmr::unsynchronized_pool_resource _tempResource{{2, 1024}};
		
		internal::SparseSet<Archetype *, EntityId> _entityToContainer{_sparseMemory};
	};
}
