#pragma once

namespace flf::internal
{
	template<typename T>
	static void DefaultConstructAt(void *at)
	{
		if constexpr(std::is_default_constructible_v<T>)
		{
			new(at) T();
		} else
		{
			assert(false); // error
		}
	}
	
	template<typename T>
	static void MoveConstructAt(void *at, void *from)
	{
		if constexpr(std::is_move_constructible_v<T>)
		{
			new(at) T(std::forward<T>(*reinterpret_cast<T *>(from)));
		} else
		{
			assert(false); // error
		}
	}
	
	template<typename T>
	static void CopyConstructAt(void *at, void *from)
	{
		if constexpr(std::is_copy_constructible_v<T>)
		{
			new(at) T(*reinterpret_cast<T *>(from));
		} else
		{
			assert(false); // error
		}
	}
	
	template<typename T>
	static void DestructAt(void *at)
	{
		if constexpr(std::is_destructible_v<T>)
		{
			reinterpret_cast<T *>(at)->~T();
		} else
		{
			assert(false); // error
		}
	}
	
	/// Collects all constructors and destructors of a type as function pointers
	struct ConstructorVTable
	{
		/// Generates the ConstructorVTable of a given type
		/// \tparam T to generate the ConstructorVTable of
		/// \return a ConstructorVTable that generates T
		template<typename T>
		constexpr static ConstructorVTable Of()
		{
			return {std::is_default_constructible_v<T> ? &DefaultConstructAt<T> : nullptr,
			        std::is_move_constructible_v<T> ? &MoveConstructAt<T> : nullptr,
			        std::is_copy_constructible_v<T> ? &CopyConstructAt<T> : nullptr,
			        std::is_destructible_v<T> ? &DestructAt<T> : nullptr};
		}
		
		void (*defaultConstruct)(void *at);
		
		void (*moveConstruct)(void *at, void *from);
		
		void (*copyConstruct)(void *at, void *from);
		
		void (*destruct)(void *at);
	};
}
