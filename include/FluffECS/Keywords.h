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


#if _MSC_VER && !__INTEL_COMPILER // yeah msvc getting the special treatment... (https://en.cppreference.com/w/cpp/language/operator_alternative)

#include <ciso646>

#endif

#define FLUFF_ASSERT( expr ) assert( expr )

namespace flf
{
	template<typename T>
	using ValueType = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
}
