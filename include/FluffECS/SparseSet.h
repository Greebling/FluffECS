#ifndef FLUFFTEST_SPARSESET_H
#define FLUFFTEST_SPARSESET_H

#include <cstddef>
#include <vector>
#include <memory_resource>
#include "Keywords.h"

namespace flf::internal
{
	template<typename T, typename TIndex = std::size_t>
	class SparseSet
	{
	public:
		explicit SparseSet(std::pmr::memory_resource &sparseResource) noexcept
				: _sparse(&sparseResource)
		{
		}
		
		inline void AddEntry(TIndex index, T val) FLUFF_NOEXCEPT
		{
			Resize(index + 1);
			_sparse[index] = val;
		}
		
		void AddRange(TIndex start, TIndex end, T fillVal) FLUFF_NOEXCEPT
		{
			Resize(end + 1);
			std::fill(_sparse.begin() + start, _sparse.begin() + end, fillVal);
		}
		
		inline void SetEntry(TIndex index, T val) noexcept
		{
			_sparse[index] = val;
		}
		
		inline void MarkAsDeleted(TIndex index) noexcept
		{
			_sparse[index] = std::numeric_limits<T>::max();
		}
		
		[[nodiscard]] inline bool Contains(TIndex index) const noexcept
		{
			return _sparse.size() > index && _sparse[index] != std::numeric_limits<T>::max();
		}
		
		[[nodiscard]] inline T operator[](TIndex index) const noexcept
		{
			return _sparse[index];
		}
		
		inline void Reserve(TIndex size) FLUFF_NOEXCEPT
		{
			_sparse.reserve(size);
		}
		
		inline void Resize(TIndex size) FLUFF_NOEXCEPT
		{
			const TIndex previousSize = _sparse.size();
			_sparse.resize(size);
			
			for(TIndex i = previousSize; i < size; ++i)
			{
				_sparse[i] = std::numeric_limits<T>::max();
			}
		}
	
	private:
		std::pmr::vector<T> _sparse;
	};
}

#endif //FLUFFTEST_SPARSESET_H
