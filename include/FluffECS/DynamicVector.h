#ifndef FLUFFTEST_DYNAMICVECTOR_H
#define FLUFFTEST_DYNAMICVECTOR_H

#include <cstddef>
#include <cassert>
#include <cstring>
#include <cmath>
#include <memory>
#include <memory_resource>
#include "Keywords.h"
#include "VirtualConstructor.h"

/// may be commented out to increase performance when NDEBUG is not defined
#define FLUF_DO_RANGE_CHECKS

namespace flf::internal
{
	/// A vector implementation that only stores bytes
	class ByteVector
	{
	protected:
		/// how many objects shall be contained at minimum
		static constexpr size_t MIN_OBJECT_COUNT = 16;
	public:
		constexpr ByteVector() noexcept = default;
		
		constexpr explicit ByteVector(std::pmr::memory_resource &resource) noexcept
				: _resource(&resource)
		{
		}
	
	public:
		/// Sets the vectors memory resource. Reallocates the current memory, if there is any
		/// \param resource
		void SetMemoryResource(std::pmr::memory_resource &resource) noexcept
		{
			if(_begin != _sizeEnd && not resource.is_equal(*_resource))
			{
				// reallocate memory
				const auto byteSize = ByteSize();
				const auto byteCapacity = ByteCapacity();
				
				void *nextMemory = resource.allocate(byteCapacity);
				std::memcpy(nextMemory, _begin, byteSize);
				_resource->deallocate(_begin, byteCapacity); // use 0 for alignment as pointer is already correctly aligned
				
				_begin = (std::byte *) nextMemory;
				_sizeEnd = _begin + byteCapacity;
			}
			
			_resource = &resource;
		}
		
		/// \return the size of contained elements in bytes
		[[nodiscard]] inline std::size_t ByteSize() const noexcept
		{
			return _sizeEnd - _begin;
		}
		
		/// \return the number of bytes reserved for the vector
		[[nodiscard]] inline std::size_t ByteCapacity() const noexcept
		{
			return _capacityEnd - _begin;
		}
		
		[[nodiscard]] void *Data() noexcept
		{
			return _begin;
		}
		
		[[nodiscard]] const void *Data() const noexcept
		{
			return _begin;
		}
		
		/// Similar to PushBack, just with using raw byte data
		/// \param data to add to the vector
		/// \param size of the data to add. Note that the size of the type already contained must be equal to size
		void EmplaceBackBytes(const void *data, std::size_t size) FLUFF_NOEXCEPT
		{
			GrowSingle(size);
			
			std::memcpy(_sizeEnd, data, size);
			_sizeEnd += size;
		}
		
		/// Similar to PushBack, just with using raw byte data
		/// \param size of the data to add. Note that the size of the type already contained must be equal to size
		void PushBackBytes(std::size_t size) FLUFF_NOEXCEPT
		{
			GrowSingle(size);
			
			_sizeEnd += size;
		}
		
		/// Similar to PushBack, just with using raw byte data. Note that this leaves the new bytes uninitialized
		/// \param size of the data to add. Note that the size of the type already contained must be equal to size
		void PushBackBytesUnsafe(std::size_t size) FLUFF_NOEXCEPT
		{
			GrowSingle(size);
			std::memset(_sizeEnd, 0, size);
			_sizeEnd += size;
		}
		
		/// Reduces the size of the vector by size bytes
		/// \param size
		void PopBackBytes(std::size_t size) noexcept
		{
			_sizeEnd -= size;
		}
		
		void *GetBytes(std::size_t offset) noexcept
		{
			return _begin + offset;
		}
	
	protected:
		
		/// Grows the vector by increasing the number of components that can be saved whose sizeof() equals byteSize by one
		/// \param byteSize of the components saved here
		inline void GrowSingle(std::size_t byteSize) FLUFF_NOEXCEPT
		{
			// grows quadratically in comparison to number of components that can be stored (not in number of pure bytes)
			const auto previousSize = ByteSize();
			const auto nextByteCapacity = NextSize(previousSize / byteSize) * (previousSize + byteSize);
			ReserveBytes(nextByteCapacity);
		}
		
		inline void ReserveBytes(std::size_t nBytes) FLUFF_NOEXCEPT
		{
			const auto previousSize = ByteSize();
			const auto nextCapacity = nBytes;
			auto *next = reinterpret_cast<std::byte *>(_resource->allocate(nextCapacity));
			
			std::memcpy(next, _begin, previousSize);
			_begin = next;
			_sizeEnd = _begin + previousSize;
			_capacityEnd = _begin + nextCapacity;
		}
		
		/// Growth policy of the vector. Returns the next power of 2 that is equal to or larger than n
		/// \param n amount we want to at least store.
		/// \return the size the capacity should grow to
		[[nodiscard]] inline static constexpr std::size_t NextSize(std::size_t n) noexcept
		{
			if(n == 0) FLUFF_UNLIKELY
			{
				return 1;
			} else
			{
				n--; // as for eg n = 32 we want to give out 32, not 64
				std::size_t next = 1;
				while(n >>= 1)
					next <<= 1;
				return next << 1;
			}
		}
	
	protected:
		std::pmr::memory_resource *_resource{};
		std::byte *_begin{};
		/// end of the current elements. note that only the element before sizeEnd is valid
		std::byte *_sizeEnd{};
		/// end of the allocated memory for the vector. note that only the byte before _capacityEnd is usable
		std::byte *_capacityEnd{};
	};
	
	// TODO: Call constructors of contained objects
	class DynamicVector : public ByteVector
	{
	public:
		constexpr DynamicVector() noexcept = default;
		
		constexpr explicit DynamicVector(std::pmr::memory_resource &resource) noexcept
				: ByteVector(resource)
		{
		}
		
		void SetContainedType()
		{
		
		}
		
		/// Calls the destructor of all elements contained in elements
		/// \tparam T
		template<typename T>
		void DestructElements() noexcept(std::is_nothrow_destructible_v<T>)
		{
			for(T *current = std::launder(reinterpret_cast<T *>(_begin)), end = std::launder(reinterpret_cast<T *>(_sizeEnd)); current < end; ++current)
			{
				std::destroy_at(current);
			}
			
			_resource->deallocate(_begin, ByteCapacity());
			
			_sizeEnd = _begin;
			_capacityEnd = _begin;
		}
		
		/// Gets the indexth element in the vector
		/// \tparam T Type of object to get
		/// \param index of element to get
		/// \return the element at given index
		template<typename T>
		[[nodiscard]] inline T &Get(std::size_t index) noexcept
		{
#ifdef FLUF_DO_RANGE_CHECKS
			assert(index < Size<T>() && "Index out of range");
#endif
			return *(std::launder(reinterpret_cast<T *>(_begin)) + index);
		}
		
		/// Gets the indexth element in the vector
		/// \tparam T Type of object to get
		/// \param index of element to get
		/// \return the element at given index
		template<typename T>
		[[nodiscard]] inline const T &Get(std::size_t index) const noexcept
		{
#ifdef FLUF_DO_RANGE_CHECKS
			assert(index < Size<T>() && "Index out of range");
#endif
			return *(std::launder(reinterpret_cast<T *>(_begin)) + index);
		}
		
		/// Gets the first element
		/// \tparam T Type of object to get
		/// \return A reference to the first element of the vector
		template<typename T>
		[[nodiscard]] inline T &Front() noexcept
		{
			return *std::launder(reinterpret_cast<T *>(_begin));
		}
		
		/// Gets the last element
		/// \tparam T Type of object to get
		/// \return A reference to the last element of the vector
		template<typename T>
		[[nodiscard]] inline T &Back() noexcept
		{
			return *(std::launder(reinterpret_cast<T *>(_sizeEnd)) - 1);
		}
		
		/// Gets the first element
		/// \tparam T Type of object to get
		/// \return A reference to the first element of the vector
		template<typename T>
		[[nodiscard]] inline const T &Front() const noexcept
		{
			return *std::launder(reinterpret_cast<const T *>(_begin));
		}
		
		/// Gets the last element
		/// \tparam T Type of object to get
		/// \return A reference to the last element of the vector
		template<typename T>
		[[nodiscard]] inline const T &Back() const noexcept
		{
			return *(std::launder(reinterpret_cast<const T *>(_sizeEnd)) - 1);
		}
		
		[[nodiscard]] inline std::byte *BackPtr() noexcept
		{
			return _sizeEnd;
		}
		
		/// Calculates how many objects of given type are contained in this vector
		/// \tparam T Type of objects
		/// \return The number of objects that are contained
		template<typename T>
		[[nodiscard]] inline std::size_t Size() const noexcept
		{
			return ByteSize() / sizeof(T);
		}
		
		/// Calculates how many objects of given type can be contained in this vector without reallocation
		/// \tparam T Type of objects
		/// \return The number of objects that can be contained without a reallocation
		template<typename T>
		[[nodiscard]] inline std::size_t Capacity() const noexcept
		{
			return ByteCapacity() / sizeof(T);
		}
		
		/// Adds an element at the back
		/// \tparam T Type of element to add
		/// \return A reference to the added element
		template<typename T>
		inline T &PushBack() FLUFF_NOEXCEPT
		{
			Reserve<T>(Size<T>() + 1);
			new(reinterpret_cast<T *>(_sizeEnd)) T();
			_sizeEnd += sizeof(T);
			return Back<T>();
		}
		
		/// Emplaces an element to the back of the vector
		/// \tparam T Type of element to add
		/// \param element to emplace
		/// \return A reference to the added element
		template<typename T>
		inline T &PushBack(T &element) FLUFF_NOEXCEPT
		{
			Reserve<T>(Size<T>() + 1);
			new(reinterpret_cast<T *>(_sizeEnd)) T(element);
			_sizeEnd += sizeof(T);
			return Back<T>();
		}
		
		/// Emplaces an element to the back of the vector
		/// \tparam T Type of element to add
		/// \param element to emplace
		/// \return A reference to the added element
		template<typename T>
		inline T &EmplaceBack(T &&element) FLUFF_NOEXCEPT
		{
			Reserve<T>(Size<T>() + 1);
			new(reinterpret_cast<T *>(_sizeEnd)) T(element);
			_sizeEnd += sizeof(T);
			return Back<T>();
		}
		
		/// Constructs an element at the back of the vector
		/// \tparam T Type of element to add
		/// \param args Constructor arguments to use
		/// \return A reference the the newly created element
		template<typename T, typename ...TArgs>
		inline T &EmplaceBack(TArgs &&...args) FLUFF_NOEXCEPT
		{
			static_assert(std::is_constructible_v<T, TArgs...>, "Invalid constructor arguments given");
			
			Reserve<T>(Size<T>() + 1);
			new(reinterpret_cast<T *>(_sizeEnd)) T(std::forward<TArgs>(args)...);
			_sizeEnd += sizeof(T);
			return Back<T>();
		}
		
		/// Call the copy constructor of T amount times, appending the copies to the end of the vector
		/// \tparam T Type of element to clone
		/// \param amount of clones to create
		/// \param prototype to clone from
		template<typename T>
		void Clone(std::size_t amount, const T &prototype) FLUFF_NOEXCEPT
		{
			const auto previousSize = Size<T>();
			ResizeUnsafe<T>(previousSize + amount);
			
			for(T *curr = std::launder(reinterpret_cast<T *>(_begin)) + previousSize; curr < std::launder(reinterpret_cast<T *>(_sizeEnd)); ++curr)
			{
				new(curr) T(prototype);
			}
		}
		
		/// Removes the last element
		/// \tparam T Destructor of that type will be called on the last element
		template<typename T>
		inline void PopBack() noexcept(std::is_nothrow_destructible_v<T>)
		{
			if(Size<T>() == 0) FLUFF_UNLIKELY
			{
				return;
			}
			
			_sizeEnd -= sizeof(T);
			std::destroy_at(reinterpret_cast<T *>(_sizeEnd));
		}
		
		/// Preallocates a given number of elements
		/// \tparam T Type to use for number of individual elements
		/// \param number of elements that shall be containable without a new allocations
		template<typename T>
		inline void Reserve(std::size_t number) FLUFF_NOEXCEPT
		{
			if(number <= Capacity<T>())
			{
				// already can contain this many elements
				return;
			}
			
			const auto previousSize = ByteSize();
			auto nextCapacity = NextSize(number) * sizeof(T);
			if(nextCapacity < MIN_OBJECT_COUNT * sizeof(T))
				nextCapacity = MIN_OBJECT_COUNT * sizeof(T);
			
			auto *next = reinterpret_cast<std::byte *>(_resource->allocate(nextCapacity));
			
			// construct the individual elements
			if constexpr (std::is_trivially_move_constructible_v<T>)
			{
				std::memcpy(next, _begin, previousSize);
			} else
			{
				for(T *target = std::launder(reinterpret_cast<T *>(next)), *targetEnd = target + (previousSize / sizeof(T)),
						    *source = std::launder(reinterpret_cast<T *>(_begin));
				    target < targetEnd; ++target, ++source)
				{
					if constexpr (std::is_move_constructible_v<T>)
					{
						// move construct
						new(target) T(std::forward<T>(*source));
					} else
					{
						// copy construct
						new(target) T(*source);
					}
				}
			}
			
			_begin = next;
			_sizeEnd = _begin + previousSize;
			_capacityEnd = _begin + nextCapacity;
		}
		
		/// Sets Size<T> = size and does a reallocation if necessary
		/// \tparam T Type to use for resizing. Its default constructor will be called for new elements
		/// \param size Number of elements to contain in vector
		template<typename T>
		void Resize(std::size_t size) FLUFF_NOEXCEPT
		{
			const auto previousSize = Size<T>();
			if(previousSize == size) FLUFF_UNLIKELY
			{
				// noop
				return;
			} else if(previousSize < size)
			{
				// add elements at newEnd
				Reserve<T>(size);
				
				// init objects
				for(T *curr = std::launder(reinterpret_cast<T *>(_begin)) + previousSize, *end = std::launder(reinterpret_cast<T *>(_begin)) + size;
				    curr < end; ++curr)
				{
#if __cplusplus > 201703L
					std::construct_at(curr);
#else
					new(curr) T();
#endif
				}
				_sizeEnd = _begin + size * sizeof(T);
			} else // previousSize > size
			{
				auto nRemovedElements = size - previousSize;
				// remove last elements
				for(T *current = reinterpret_cast<T *>(_sizeEnd) - nRemovedElements;
				    current < reinterpret_cast<T *>(_sizeEnd); ++current)
				{
					std::destroy_at(current);
				}
				_sizeEnd = _begin + size * sizeof(T);
			}
		}
		
		/// Sets Size<T> = size and does a reallocation if necessary.
		/// WARNING: This does not call constructors or destructors, meaning that you will be accessing unallocated memory.
		/// However, this method is considerably faster for large sizes
		/// \tparam T Type to use for resizing. Its default constructor will be called for new elements
		/// \param size Number of elements to contain in vector
		template<typename T>
		void ResizeUnsafe(std::size_t size) FLUFF_NOEXCEPT
		{
			auto previousSize = Size<T>();
			if(previousSize == size) FLUFF_UNLIKELY
			{
				// noop
				return;
			} else if(previousSize < size)
			{
				// add elements at end
				Reserve<T>(size);
				_sizeEnd = _begin + size * sizeof(T);
			} else // previousSize > size
			{
				_sizeEnd = _begin + size * sizeof(T);
			}
		}
	};
}

#endif //FLUFFTEST_DYNAMICVECTOR_H
