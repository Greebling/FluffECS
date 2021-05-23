#pragma once

namespace flf
{
	using EntityId = std::uint32_t;
	
	namespace internal
	{
		class WorldInternal;
	}
	
	
	class Entity
	{
	public:
		constexpr explicit Entity() FLUFF_NOEXCEPT = default;
		
		constexpr Entity(const Entity &) FLUFF_NOEXCEPT = default;
		
		constexpr Entity(Entity &&) FLUFF_NOEXCEPT = default;
		
		Entity &operator=(const Entity &) FLUFF_NOEXCEPT = default;
		
		Entity &operator=(Entity &&) FLUFF_NOEXCEPT = default;
		
		constexpr Entity(EntityId id, internal::WorldInternal &container) FLUFF_NOEXCEPT
				: _id(id), _world(&container)
		{
		}
	
	public:
		[[nodiscard]] constexpr inline EntityId Id() const FLUFF_NOEXCEPT
		{
			return _id;
		}
		
		/// Gets the component belonging to that entity
		/// \tparam TComponent to get
		/// \return a reference to the component
		template<typename TComponent>
		[[nodiscard]] inline TComponent *Get() FLUFF_NOEXCEPT;
		
		/// Gets the component belonging to that entity
		/// \tparam TComponent to get
		/// \return a reference to the component
		template<typename TComponent>
		[[nodiscard]] inline const TComponent *Get() const FLUFF_NOEXCEPT;
		
		/// Checks whether the entity has a given component type associated with it
		/// \tparam TComponent to check
		/// \return true when this entity has the given component, false otherwise
		template<typename TComponent>
		[[nodiscard]] inline bool Has() const FLUFF_NOEXCEPT;
		
		/// Checks whether this entity is still existing in the world
		[[nodiscard]] inline bool IsDead() const FLUFF_NOEXCEPT;
		
		/// Kills the entity, destroying all of its components
		inline void Destroy() FLUFF_MAYBE_NOEXCEPT;
	
	private:
		EntityId _id = 0;
		
		/// used for get and destroy helper functions
		internal::WorldInternal *_world = nullptr;
	};
}
