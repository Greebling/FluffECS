#ifndef FLUFF_ECS_TYPELIST_H
#define FLUFF_ECS_TYPELIST_H

#include <utility>
#include <type_traits>

#include "TypeId.h"

namespace flf::internal
{
	// Original Author: Björn Fahller (https://playfulprogramming.blogspot.com/2017/06/constexpr-quicksort-in-c17.html), with minor tweaks done by myself
	
	template<typename T, typename ...Ts>
	struct FrontOf
	{
		using Type = T;
	};
	
	template<typename ...Ts>
	struct TypeList
	{
	};
	
	template<typename T>
	struct TypeContainer
	{
		using type = T;
	};
	
	template<typename TRemove, typename T, typename ...Ts>
	constexpr auto Remove(TypeList<T, Ts...>)
	{
		if constexpr (std::is_same_v<T, TRemove>)
		{
			return TypeList<Ts...>();
		} else
		{
			if constexpr ((std::is_same_v<TRemove, Ts> || ...))
			{
				return Remove<TRemove>(TypeList<Ts..., T>());
			} else
			{
				// Type is not in list
				return TypeList<T, Ts...>();
			}
		}
	}
	
	template<typename T, typename ... Ts>
	constexpr TypeContainer<T> Front(TypeList<T, Ts...>)
	{
		return TypeContainer<T>{};
	}
	
	template<typename T, typename ... Ts>
	constexpr TypeList<Ts...> AllExceptFront(TypeList<T, Ts...>)
	{
		return TypeList<Ts...>{};
	}
	
	template<typename ... Ts, typename ... Us>
	constexpr TypeList<Ts..., Us...> operator|(TypeList<Ts...>, TypeList<Us...>)
	{
		return TypeList<Ts..., Us...>{};
	}
	
	template<typename P, typename ... Ts>
	constexpr auto Partition(TypeList<P> pivot, TypeList<Ts...> tl)
	{
		if constexpr (sizeof...(Ts) == 0)
		{
			return std::pair(TypeList{}, TypeList{});
		} else
		{
			using THead = decltype(Front(tl));
			constexpr auto r = Partition(pivot, AllExceptFront(tl));
			if constexpr (TypeId < typename THead::type > < TypeId < P >)
			{
				return std::pair(TypeList<typename THead::type>{} | r.first, r.second);
			} else
			{
				return std::pair(r.first, TypeList<typename THead::type>{} | r.second);
			}
		}
	}
	
	
	template<typename ... Ts>
	constexpr auto Sort(TypeList<Ts...> list)
	{
		if constexpr (sizeof...(Ts) <= 1)
		{
			return TypeList<Ts...>{};
		} else
		{
			using TPivot = decltype(Front(list));
			constexpr auto r = Partition(TypeList<typename TPivot::type>{}, AllExceptFront(list));
			return Sort(r.first) | TypeList<typename TPivot::type>{} | Sort(r.second);
		}
	}
}

#endif //FLUFF_ECS_TYPELIST_H
