#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <string_view>
#include <array>

#include "Keywords.h"

/// Activates additional name member for flf::TypeInformation, especially helpful for debugging
//#define FLUFF_TYPE_INFO_NAME

namespace flf
{
	using IdType = std::uint32_t;
	using MultiIdType = IdType;
	
	namespace internal
	{
		template<typename T>
		constexpr auto GetFuncName()
		{
#if defined __clang__ || defined __GNUC__
			return __PRETTY_FUNCTION__;
#elif _MSC_VER && !__INTEL_COMPILER
			return __FUNCSIG__;
#else // not compatible with other compilers
			return "";
#endif
		}
		
		template<typename T>
		constexpr auto GetTypeName()
		{
			constexpr std::string_view str = GetFuncName<T>();
#if defined __clang__ || defined __GNUC__
			if constexpr(str.empty())
			{
				return str;
			} else
			{
				constexpr auto lPos = str.find('=') + 2;
				constexpr auto rPos = str.rfind(']');
				return str.substr(lPos, (rPos - lPos));
			}
#elif _MSC_VER && !__INTEL_COMPILER
			constexpr auto lPos = str.find('<') + 1;
			constexpr auto rPos = str.rfind('>');
			return str.substr(lPos, (rPos - lPos));
#else
			static_assert(false, "Compiler does not support conversion from type to string.");
			return std::string_view();
#endif
		}
		
		constexpr inline IdType HashString(std::string_view str, IdType basis = 2166136261u) FLUFF_NOEXCEPT
		{
			if (str.empty())
			{
				return basis;
			} else
			{
				auto currChar = (IdType) ((unsigned char) str.front());
				return HashString(str.substr(1, str.length() - 1), (basis xor currChar) * 16777619u);
			}
		}
		
		/// Creates a unique IdType for every type
		template<typename T>
		constexpr inline IdType GenerateTypeId() FLUFF_NOEXCEPT
		{
			return HashString(GetTypeName<T>());
		}
	}
	
	
	template<typename T>
	constexpr IdType TypeId()
	{
		return internal::GenerateTypeId<T>();
	}
	
	/// Gives the same id for any combination of the same Ts
	/// \tparam Ts the types of the id
	template<typename ...Ts>
	constexpr IdType MultiTypeId()
	{
		if constexpr(sizeof...(Ts) == 0)
		{
			return 0;
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
#ifdef FLUFF_TYPE_INFO_NAME
		std::string_view name{};
#endif
	
	public:
		template<typename T>
		static constexpr TypeInformation Of() FLUFF_NOEXCEPT
		{
			return TypeInformation(TypeId<T>(),
			                       sizeof(T)
#ifdef FLUFF_TYPE_INFO_NAME
					,internal::GetTypeName<T>()
#endif
			);
		}
		
		constexpr TypeInformation(IdType id, std::size_t size) FLUFF_NOEXCEPT
				: id(id), size(size)
		{
		}

#ifdef FLUFF_TYPE_INFO_NAME
		constexpr TypeInformation(IdType id, std::size_t size, std::string_view name) FLUFF_NOEXCEPT
				: id(id),
				size(size),
				name(name)
		{
		}
#endif
		
		TypeInformation() FLUFF_NOEXCEPT = default;
	};
	
	namespace internal
	{
		
		template<typename TIterator1, typename TIterator2>
		static MultiIdType CombineIds(TIterator1 begin1, TIterator1 end1, TIterator2 begin2, TIterator2 end2)
		{
			MultiIdType result = MultiTypeId<>();
			
			for (; begin1 < end1; begin1++)
			{
				result = result xor *begin1;
			}
			
			for (; begin2 < end2; begin2++)
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
