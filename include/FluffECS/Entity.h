#ifndef FLUFF_ECS_ENTITY_H
#define FLUFF_ECS_ENTITY_H

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
		constexpr explicit Entity() noexcept = default;
		
		constexpr Entity(const Entity &) noexcept = default;
		
		constexpr Entity(Entity &&) noexcept = default;
		
		Entity &operator=(const Entity &) noexcept = default;
		
		Entity &operator=(Entity &&) noexcept = default;
		
		constexpr Entity(EntityId id, internal::WorldInternal &container) noexcept
				: _id(id), _world(&container)
		{
		}
	
	public:
		[[nodiscard]] constexpr inline EntityId Id() const noexcept
		{
			return _id;
		}
		
		/// Gets the component belonging to that entity
		/// \tparam TComponent to get
		/// \return a reference to the component
		template<typename TComponent>
		[[nodiscard]] inline TComponent *Get() noexcept;
		
		/// Gets the component belonging to that entity
		/// \tparam TComponent to get
		/// \return a reference to the component
		template<typename TComponent>
		[[nodiscard]] inline const TComponent *Get() const noexcept;
		
		/// Checks whether the entity has a given component type associated with it
		/// \tparam TComponent to check
		/// \return true when this entity has the given component, false otherwise
		template<typename TComponent>
		[[nodiscard]] inline bool Has() const noexcept;
		
		inline void Destroy() FLUFF_NOEXCEPT;
	
	private:
		EntityId _id = 0;
		
		/// used for get and destroy helper functions
		internal::WorldInternal *_world = nullptr;
	};
}

#endif //FLUFF_ECS_ENTITY_H
