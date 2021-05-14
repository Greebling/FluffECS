#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

#if _MSC_VER && !__INTEL_COMPILER // yeah msvc getting the special treatment... (https://en.cppreference.com/w/cpp/language/operator_alternative)
#include <ciso646>
#endif

namespace flf
{
	using IdType = std::uint32_t;
	using MultiIdType = std::uint32_t;
	
	namespace internal
	{
		constexpr inline IdType
		HashString(const char *str, std::size_t stringSize, std::uint32_t basis = 2166136261u) noexcept
		{
			if (stringSize == 0)
			{
				return basis;
			} else
			{
				return HashString(str + 1, stringSize - 1, (basis xor (IdType) str[0]) * 16777619u);
			}
		}
		
		/// Creates a unique IdType for every type
		template<typename T>
		constexpr inline IdType TypeIdGenerator() noexcept
		{
#if defined __clang__ || defined __GNUC__
			return HashString(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__));
#elif _MSC_VER && !__INTEL_COMPILER
			return HashString(__FUNCSIG__, sizeof(__FUNCSIG__));
#else // not compatible with other compilers
			static_assert(false, "constexpr TypeID is not supported by your Compiler! Choose either Clang, GCC or MSVC");
			return 0;
#endif
		}
	}
	
	
	template<typename T>
	constexpr IdType TypeId()
	{
		return internal::TypeIdGenerator<T>();
	}
	
	/// Gives the same id for any combination of the same Ts
	/// \tparam Ts the types of the id
	template<typename ...Ts>
	constexpr IdType MultiTypeId()
	{
		if constexpr(sizeof...(Ts) == 0)
		{
			return 0;
		}
		else if constexpr(sizeof...(Ts) == 1)
		{
			return MultiTypeId<>() xor (..., TypeId<Ts>()); // trick to simply return TypeId<T>()
		} else
		{
			return MultiTypeId<>() xor (TypeId<Ts>() xor ...);
		}
	}
	
	struct TypeInformation
	{
	public:
		IdType id = 0;
		std::size_t size = 0;
	
	public:
		template<typename T>
		static constexpr TypeInformation Of() noexcept
		{
			return TypeInformation(TypeId<T>(), sizeof(T));
		}
		
		constexpr TypeInformation(IdType id, std::size_t size) noexcept
				: id(id), size(size)
		{
		}
		
		TypeInformation() noexcept = default;
	};
	
	namespace internal
	{
		template<typename TIterator1, typename TIterator2>
		static MultiIdType CombineIds(TIterator1 begin1, TIterator1 end1, TIterator2 begin2, TIterator2 end2)
		{
			MultiIdType result = MultiTypeId<>();
			
			for(;begin1 < end1; begin1++)
			{
				result = result xor *begin1;
			}
			
			for(;begin2 < end2; begin2++)
			{
				result = result xor *begin2;
			}
			
			return result;
		}
		
		template<typename TIterator>
		static MultiIdType CombineIds(TIterator begin, TIterator end)
		{
			MultiIdType result = MultiTypeId<>();
			for (; begin < end; ++begin)
			{
				result = result xor *begin;
			}
			
			return result;
		}
	}
}
