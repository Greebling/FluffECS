#ifndef FLUFF_ECS_TYPEID_H
#define FLUFF_ECS_TYPEID_H

#include <cstdint>
#include <cstddef>
#include <type_traits>

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
				return HashString(str + 1, stringSize - 1, (basis xor (unsigned int) str[0]) * 16777619u);
			}
		}
		
		/// Creates a unique IdType for every type
		template<typename T>
		constexpr inline IdType TypeIdGenerator() noexcept
		{
#if defined __clang__ || defined __GNUC__
			return HashString(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__));
#elif defined _MSC_VER
			return HashString(__FUNCSIG__, sizeof(__FUNCSIG__));*?
#else // not compatible with other compilers
			static_assert(false, "constexpr TypeID is not supported by your Compiler! Choose either Clang, GCC or MSVC");
			return 0;
#endif
		}
		
		template<IdType ...ids>
		static constexpr MultiIdType CombineIds()
		{
			return ((ids + 0x9e3779b9) xor ... xor 0) + (ids + ... + 0);
		}
		
		template<typename TIterator>
		static MultiIdType CombineIds(TIterator begin, TIterator end, IdType typeToAdd)
		{
			MultiIdType result = 0;
			TIterator beginXor = begin;
			for (; beginXor < end; ++beginXor)
			{
				result = result xor (*beginXor + 0x9e3779b9);
			}
			result = result xor (typeToAdd + 0x9e3779b9);
			
			for (; begin < end; ++begin)
			{
				result += *begin;
			}
			result += typeToAdd;
			
			return result;
		}
		
		template<typename TIterator1, typename TIterator2>
		static MultiIdType CombineIds(TIterator1 begin1, TIterator1 end1, TIterator2 begin2, TIterator2 end2)
		{
			MultiIdType result = 0;
			TIterator1 curr1 = begin1;
			for (; curr1 < end1; ++curr1)
			{
				result = result xor (*curr1 + 0x9e3779b9);
			}
			TIterator2 curr2 = begin2;
			for (; curr2 < end2; ++curr2)
			{
				result = result xor (*curr2 + 0x9e3779b9);
			}
			
			for (; begin1 < end1; ++begin1)
			{
				result += *begin1;
			}
			for (; begin2 < end2; ++begin2)
			{
				result += *begin2;
			}
			
			return result;
		}
		
		template<typename TIterator>
		static MultiIdType CombineIds(TIterator begin, TIterator end)
		{
			MultiIdType result = 0;
			TIterator beginXor = begin;
			for (; beginXor < end; ++beginXor)
			{
				result = result xor (*beginXor + 0x9e3779b9);
			}
			
			for (; begin < end; ++begin)
			{
				result += *begin;
			}
			
			return result;
		}
	}
	
	
	template<typename T>
	std::integral_constant<IdType, internal::TypeIdGenerator<T>()> TypeId;
	
	
	/// Gives the same id for any combination of the same Ts
	/// \tparam Ts the types of the id
	template<typename ...Ts>
	std::integral_constant<MultiIdType, internal::CombineIds<TypeId<Ts>...>()> MultiTypeId;
	
	struct TypeInformation
	{
	public:
		IdType id = 0;
		std::size_t size = 0;
	
	public:
		template<typename T>
		static constexpr TypeInformation Of() noexcept
		{
			return TypeInformation(TypeId<T>, sizeof(T));
		}
		
		constexpr TypeInformation(IdType id, std::size_t size) noexcept
				: id(id), size(size)
		{
		}
		
		TypeInformation() noexcept = default;
	};
}

#endif //FLUFF_ECS_TYPEID_H
