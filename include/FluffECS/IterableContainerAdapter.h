#pragma once

#include <iterator>
#include <array>
#include <tuple>

#include "ComponentContainer.h"
#include "TypeList.h"

namespace flf
{
	template<typename ...TComponents>
	class IteratorContainer;
	
	/// Implements begin and end functions to make a ComponentContainer usable for iterations
	/// \tparam TComponents that shall be iterated over
	template<typename ...TComponents>
	class IterableContainerAdapter
	{
		static_assert((!std::is_pointer_v<TComponents> && ...), "Type cannot be a pointer");
	public:
		explicit IterableContainerAdapter(ComponentContainer &container) FLUFF_NOEXCEPT
				: _container(container)
		{
		}
		
		explicit IterableContainerAdapter(const ComponentContainer &container) FLUFF_NOEXCEPT
				: _container(const_cast<ComponentContainer &>(container))
		{
		}
		
		IterableContainerAdapter() FLUFF_NOEXCEPT = default;
	
	private:
		ComponentContainer &_container;
	
	public:
		using Iterator = ComponentContainer::Iterator<TComponents...>;
		using ReverseIterator = std::reverse_iterator<Iterator>;
		
		inline Iterator begin() FLUFF_NOEXCEPT
		{
			return Iterator(_container, 0);
		}
		
		inline Iterator end() FLUFF_NOEXCEPT
		{
			return Iterator(_container, _container.Size());
		}
		
		inline ReverseIterator rbegin() FLUFF_NOEXCEPT
		{
			return ReverseIterator(_container, 0);
		}
		
		inline ReverseIterator rend() FLUFF_NOEXCEPT
		{
			return ReverseIterator(_container, _container.Size());
		}
	};
}
