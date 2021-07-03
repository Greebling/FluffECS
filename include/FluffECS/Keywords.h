#pragma once

#define FLUFF_NOEXCEPT noexcept

#define FLUFF_MAYBE_NOEXCEPT noexcept

#if __cplusplus > 201703L
/// C++20 features
#define FLUFF_LIKELY [[likely]]
#define FLUFF_UNLIKELY [[unlikely]]
#else
// Not defined before C++20
#define FLUFF_LIKELY
#define FLUFF_UNLIKELY
#endif


#include <type_traits>
#include <cassert>

#define FLUFF_ASSERT( expr ) assert( expr )

namespace flf
{
	template<typename T>
	using ValueType = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
	
	namespace internal
	{
#ifdef FLUFF_DISABLE_EMPTY_TYPE_OPTIMIZATION
		template<typename T>
		constexpr bool IsEmpty = false;
#else
		template<typename T>
		constexpr bool IsEmpty = std::is_empty_v<T> && std::is_trivially_constructible_v<T>;
#endif
	}
}
