#pragma once

#include <utility>
#include <type_traits>

#include "TypeId.h"

namespace flf::internal
{
	
	template<typename T>
	constexpr bool IsEmpty = std::is_empty_v<T> && std::is_trivially_constructible_v<T>;
	
	template<typename T, typename ...Ts>
	struct FirstNonEmptyChecker
	{
		using Type = std::conditional_t<IsEmpty<T>, typename FirstNonEmptyChecker<Ts...>::Type, T>;
	};
	
	template<typename T>
	struct FirstNonEmptyChecker<T>
	{
		// TODO: Add compilation error at !std::is_empty_v<T>
		//static_assert(!std::is_empty_v<T>, "At least one type has to be non empty");
		using Type = std::conditional_t<IsEmpty<T>, void, T>;
	};
	
	/// Alias for the first nonempty type of template arguments or, if all types are empty, void alias
	template<typename ...Ts>
	using FirstNonEmpty = typename FirstNonEmptyChecker<Ts...>::Type;
	
	
	template<typename T, typename ...Ts>
	struct FrontOf
	{
		using Type = T;
	};
	
	template<typename ...Ts>
	struct TypeList
	{
		static constexpr std::size_t Size()
		{
			return sizeof...(Ts);
		}
	};
	
	template<typename T>
	struct TypeContainer
	{
		using type = T;
	};
	
	/// Removes the first type of a list
	template<typename T, typename ...Ts>
	constexpr TypeList<Ts...> RemoveFirst(TypeList<T, Ts...>)
	{
		return {};
	}
	
	// Original Author of type list sorting: Björn Fahller (https://playfulprogramming.blogspot.com/2017/06/constexpr-quicksort-in-c17.html), with minor tweaks done by myself
	
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
			if constexpr (TypeId<typename THead::type>() < TypeId<P>())
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
	
	template<typename Front, typename ...Ts, typename ...TDone>
	constexpr auto AllNonEmptyTypes(TypeList<Front, Ts...>, TypeList<TDone...> = TypeList<>())
	{
		if constexpr (sizeof...(Ts) == 0)
		{
			if constexpr(IsEmpty<Front>)
			{
				return TypeList<TDone...>();
			} else
			{
				return TypeList<TDone..., Front>();
			}
		} else
		{
			if constexpr(IsEmpty<Front>)
			{
				return AllNonEmptyTypes(TypeList<Ts...>(), TypeList<TDone...>());
			} else
			{
				return AllNonEmptyTypes(TypeList<Ts...>(), TypeList<TDone..., Front>());
			}
		}
	}
	
	template<typename ...Ts>
	constexpr std::array<IdType, sizeof...(Ts)> TypeIdsFromList(TypeList<Ts...>)
	{
		return {TypeId<Ts>()...};
	}
	
	template<typename T, typename ...Ts>
	void PrintTypes(internal::TypeList<T, Ts...>)
	{
		constexpr std::string_view name = internal::GetTypeName<T>();
		std::puts(std::string{&name.at(0), name.length()}.c_str());
		
		if constexpr(sizeof...(Ts) != 0)
		{
			PrintTypes<Ts...>(internal::TypeList<Ts...>());
		}
	}
	
	template<typename MemberPtrT>
	struct FunctionTraits
	{
	};
	
	template<typename TResult, typename TClass, typename ...TArgs>
	struct FunctionTraits<TResult (TClass::*)(TArgs...)>
	{
		using Result = TResult;
		using Arguments = TypeList<TArgs...>;
	};
	
	template<typename TResult, typename TClass, typename ...TArgs>
	struct FunctionTraits<TResult (TClass::*)(TArgs...) const>
	{
		using Result = TResult;
		using Arguments = TypeList<TArgs...>;
	};
	
	/// Generates a TypeList<FunctionArgTypes...> from any given callable
	/// \tparam T type of the callable
	/// \return TypeList<Ts...> of all arguments
	template<typename T>
	constexpr auto CallableArgList(T&)
	{
		using ArgumentTypes =  typename FunctionTraits<decltype(&T::operator())>::Arguments;
		return ArgumentTypes{};
	}
}

namespace flf
{
	template<typename ...Ts>
	constexpr std::array<IdType, sizeof ...(Ts)> SortedTypeIdList()
	{
		return SortedTypeIdListImpl(internal::Sort(internal::TypeList<Ts...>()));
	}
	
	namespace internal
	{
		template<typename ...Ts>
		constexpr std::array<IdType, sizeof ...(Ts)> SortedTypeIdListImpl(TypeList<Ts...>)
		{
			return {TypeId<Ts>()...};
		}
	}
}
