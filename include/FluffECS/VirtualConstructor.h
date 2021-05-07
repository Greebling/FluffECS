#ifndef FLUFFTEST_VIRTUALCONSTRUCTOR_H
#define FLUFFTEST_VIRTUALCONSTRUCTOR_H

namespace flf::internal
{
	template<typename T>
	static void DefaultConstructAt(void *at)
	{
		new(at) T();
	}
	
	template<typename T>
	static void MoveConstructAt(void *at, void *from)
	{
		new(at) T(std::forward<T>(*reinterpret_cast<T *>(from)));
	}
	
	template<typename T>
	static void CopyConstructAt(void *at, void *from)
	{
		new(at) T(*reinterpret_cast<T *>(from));
	}
	
	template<typename T>
	static void DestructAt(void *at)
	{
		reinterpret_cast<T *>(at)->~T();
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
			static_assert(std::is_default_constructible_v<T> && std::is_move_constructible_v<T> &&
			              std::is_copy_constructible_v<T> && std::is_destructible_v<T>);
			
			return {&DefaultConstructAt<T>, &MoveConstructAt<T>, &CopyConstructAt<T>, &DestructAt<T>};
		}
		
		void (*defaultConstruct)(void *at);
		
		void (*moveConstruct)(void *at, void *from);
		
		void (*copyConstruct)(void *at, void *from);
		
		void (*destruct)(void *at);
	};
}

#endif //FLUFFTEST_VIRTUALCONSTRUCTOR_H
