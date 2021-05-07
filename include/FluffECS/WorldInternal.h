#ifndef FLUFFTEST_WORLDINTERNAL_H
#define FLUFFTEST_WORLDINTERNAL_H

#include <memory_resource>
#include <utility>
#include "Keywords.h"
#include "SparseSet.h"
#include "Entity.h"

namespace flf
{
	class ComponentContainer;
}

namespace flf::internal
{
	class WorldInternal
	{
	public:
		[[nodiscard]] bool Contains(EntityId id) const noexcept
		{
			return _entityToContainer.Contains(id);
		}
		
		[[nodiscard]] ComponentContainer &ContainerOf(EntityId id) noexcept
		{
			return *_entityToContainer[id];
		}
		
		[[nodiscard]] const ComponentContainer &ContainerOf(EntityId id) const noexcept
		{
			return *_entityToContainer[id];
		}
		
		inline std::pair<EntityId, EntityId> GetNextIndicesRange(EntityId n, ComponentContainer &owner)
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
		inline EntityId TakeNextFreeIndex(ComponentContainer &owner) FLUFF_NOEXCEPT
		{
			auto index = _nextFreeIndex;
			// TODO: can we make this more efficient than taking most of the time of creating many entities?
			_entityToContainer.AddEntry(index, &owner);
			
			++_nextFreeIndex;
			return index;
		}
		
		inline void AssociateIdWith(EntityId id, ComponentContainer &container) noexcept
		{
			_entityToContainer.SetEntry(id, &container);
		}
	
	protected:
		EntityId _nextFreeIndex = 0;
		
		std::pmr::monotonic_buffer_resource _sparseMemory{8192};
		/// Used for small, temporary allocations
		std::pmr::unsynchronized_pool_resource _tempResource{{2, 1024}};
		
		internal::SparseSet<ComponentContainer *, EntityId> _entityToContainer{_sparseMemory};
	};
}

#endif //FLUFFTEST_WORLDINTERNAL_H
