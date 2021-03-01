#ifndef FLUFFTEST_ITERATORCONTAINER_H
#define FLUFFTEST_ITERATORCONTAINER_H

#include <utility>

#include "Entity.h"
#include "IterableContainerAdapter.h"

namespace flf
{
	/// Contains multiple iterators pointing to the start of differenct component container all containing the given types
	/// \tparam TComponents the containers of the iterators at least have
	template<typename ...TComponents>
	class IteratorContainer
	{
	public:
		using Iterator = ComponentContainer::Iterator<TComponents...>;
	public:
		IteratorContainer(std::vector<Iterator> &&containers, EntityId index)
				: iterators(std::forward<std::vector<Iterator>>(containers)),
				  _currIndex(index)
		{
		}
	
	private:
		template<typename T>
		friend
		class BasicWorld;
		
		std::vector<Iterator> iterators;
		EntityId _currIndex;
	
	public:
		auto Size() const noexcept
		{
			return iterators.size();
		}
	};
	
	template<typename ...TComponents>
	class MultiContainerRange
	{
	public:
		using Iterator = ComponentContainer::Iterator<TComponents...>;
	public:
		explicit MultiContainerRange(std::vector<ComponentContainer *> &&containers)
				: _containers(std::move(containers))
		{
		}
		
		IteratorContainer<TComponents...> begin() FLUFF_NOEXCEPT
		{
			std::vector<Iterator> iterators{};
			
			for(auto *container : _containers)
			{
				if(container->Size() != 0)
					iterators.emplace_back(IterableContainerAdapter<TComponents...>(*container).begin());
			}
			
			return IteratorContainer<TComponents...>(std::forward<std::vector<Iterator>>(iterators), 0);
		}
		
		IteratorContainer<TComponents...> end() FLUFF_NOEXCEPT
		{
			std::vector<Iterator> iterators{};
			
			for(auto *container : _containers)
			{
				if(container->Size() != 0)
					iterators.emplace_back(IterableContainerAdapter<TComponents...>(*container).end());
			}
			
			return IteratorContainer<TComponents...>(std::forward<std::vector<Iterator>>(iterators), iterators.size());
		}
	
	
	private:
		std::vector<ComponentContainer *> _containers;
	};
	
	/// Begins the iteration over a MultiContainerRange, using given names as iteration variables
#define FLF_FOR(ENTITIES, ...) \
    {\
        auto _fluff_iterateBegin = ENTITIES.begin();\
        for(flf::EntityId i = 0; i < _fluff_iterateBegin.Size(); ++i)\
        {\
            auto _fluff_currentIterator = _fluff_iterateBegin.iterators[i];\
            while(!_fluff_currentIterator.IsOverEnd())\
            {\
                    auto[ __VA_ARGS__ ] = *_fluff_currentIterator;
	
	/// Ends an iteration
#define FLF_FOR_END() \
            ++_fluff_currentIterator;\
            }\
        }\
    }
	
	
}

#endif //FLUFFTEST_ITERATORCONTAINER_H
