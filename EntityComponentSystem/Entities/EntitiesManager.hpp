#ifndef ENTITIES_MANAGER
#define ENTITIES_MANAGER

#include "EntitiesObjectPool.hpp"


#include <typeinfo>
#include <algorithm>

namespace ecs
{	
	template <std::size_t CAPACITY>
	class EntitiesManager
	{
	public:
		class Entity;

		[[nodiscard]] constexpr Entity requestEntity() noexcept(false);

		[[nodiscard]] constexpr bool isFull() const noexcept;

		class Entity
		{
		public:
			EntityId getId() const noexcept;

			template <ComponentClassConcept ComponentClass>
			[[nodiscard]] constexpr bool hasComponent() const noexcept;

			template <ComponentClassConcept ComponentClass>
			[[nodiscard]] constexpr PooledComponentVariant& getComponent() noexcept;

			template <ComponentClassConcept ComponentClass>
			[[nodiscard]] constexpr bool addComponent() noexcept;

			template <ComponentClassConcept ComponentClass>
			[[nodiscard]] constexpr bool removeComponent() noexcept;

			template <Group group>
			[[nodiscard]] constexpr bool isMemberOf() const noexcept;

			template <Group group>
			[[nodiscard]] constexpr bool enrollToGroup() noexcept;

			template <Group group>
			[[nodiscard]] constexpr bool dismissFromGroup() noexcept;


		private:
			friend EntitiesManager<CAPACITY>;

			EntitiesManager& entitiesManager_;
			PooledEntityBody pooledEntityBody_;

			explicit Entity(EntitiesManager& entitiesManager);
		};


	private:
		// systems
		friend constexpr void move_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);
		friend constexpr void decrease_lifetime_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);
		friend constexpr void dummy_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);

		static EntityId nextId_s;

		ComponentObjectPool<PhysicsComponent, CAPACITY> physicsComponentsPool_;
		ComponentObjectPool<LifetimeComponent, CAPACITY> lifetimeComponentsPool_;

		EntitiesObjectPool<CAPACITY> entitiesPool_;

		std::mutex mu_;
	};


	//////// EntitiesManager definitions //////// 
	template <std::size_t CAPACITY>
	EntityId EntitiesManager<CAPACITY>::nextId_s{ 0U };

	template <std::size_t CAPACITY>
	constexpr EntitiesManager<CAPACITY>::Entity EntitiesManager<CAPACITY>::requestEntity() noexcept(false)
	{
		std::lock_guard lock{ mu_ };

		Entity ent{ *this };
		ent.pooledEntityBody_->id = EntitiesManager<CAPACITY>::nextId_s++;
		return ent;
	}

	template <std::size_t CAPACITY>
	constexpr bool EntitiesManager<CAPACITY>::isFull() const noexcept
	{
		return entitiesPool_.isFull();
	}

	//////// Entity definitions //////// 
	template <std::size_t CAPACITY>
	EntityId EntitiesManager<CAPACITY>::Entity::getId() const noexcept
	{
		return pooledEntityBody_->id;
	}

	template <std::size_t CAPACITY>
	EntitiesManager<CAPACITY>::Entity::Entity(EntitiesManager& entitiesManager)
		: entitiesManager_{ entitiesManager }
		, pooledEntityBody_{ entitiesManager_.entitiesPool_.request() }
	{ }

	template <std::size_t CAPACITY>
	template <ComponentClassConcept ComponentClass>
	constexpr bool EntitiesManager<CAPACITY>::Entity::hasComponent() const noexcept
	{
		for (const PooledComponentVariant& component : pooledEntityBody_->components)
		{
			if (std::holds_alternative<PooledComponent<ComponentClass>>(component))
			{
				return true;
			}
		}

		return false;
	}

	template <std::size_t CAPACITY>
	template <ComponentClassConcept ComponentClass>
	constexpr PooledComponentVariant& EntitiesManager<CAPACITY>::Entity::getComponent() noexcept
	{
		// notice that due to EntityBody's definition if the wanted component isn't contained,
		// then its alternative is a std::monostate

		std::size_t firstEmpty{ componentClassesCount };
		for (std::size_t i{ 0U }; i != componentClassesCount; ++i)
		{
			if (std::holds_alternative<PooledComponent<ComponentClass>>(pooledEntityBody_->components[i]))
			{
				return pooledEntityBody_->components[i];
			}
			else if (firstEmpty == componentClassesCount &&
				std::holds_alternative<std::monostate>(pooledEntityBody_->components[i]))
			{
				firstEmpty = i;
			}
		}

		return pooledEntityBody_->components[firstEmpty];
	}

	template <std::size_t CAPACITY>
	template <ComponentClassConcept ComponentClass>
	constexpr bool EntitiesManager<CAPACITY>::Entity::addComponent() noexcept
	{
		std::size_t firstEmpty{ componentClassesCount };
		for (std::size_t i{ 0U }; i != componentClassesCount; ++i)
		{
			if (std::holds_alternative<PooledComponent<ComponentClass>>(pooledEntityBody_->components[i]))
			{
				return false;
			}
			else if (firstEmpty == componentClassesCount &&
				std::holds_alternative<std::monostate>(pooledEntityBody_->components[i]))
			{
				firstEmpty = i;
			}
		}

		// if we've reached here than firstEmpty != componentClassesCount.
		// proof by negation: assume firstEmpty == componentClassesCount then
		// none of pooledEntityBody_->components is std::monostate, hence some variant 
		// is already the component the client wished to add, so we've returned false

		// notice ComponentClass is never polymorphic, therefore "typeid(ComponentClass)" 
		// is resolved at compile time, without additional runtime overhead, see "Notes" at -
		// https://en.cppreference.com/w/cpp/language/typeid

		if (typeid(ComponentClass) == typeid(PhysicsComponent))
		{
			PooledComponent<PhysicsComponent> physComp = entitiesManager_.physicsComponentsPool_.request();
			physComp->valid = true;
			pooledEntityBody_->components[firstEmpty] = std::move(physComp);
		}
		else if (typeid(ComponentClass) == typeid(LifetimeComponent))
		{
			PooledComponent<LifetimeComponent> lifetimeComp = entitiesManager_.lifetimeComponentsPool_.request();
			lifetimeComp->valid = true;
			pooledEntityBody_->components[firstEmpty] = std::move(lifetimeComp);
		}
		else
		{
			return false;
		}

		return true;
	}

	template <std::size_t CAPACITY>
	template <ComponentClassConcept ComponentClass>
	constexpr bool EntitiesManager<CAPACITY>::Entity::removeComponent() noexcept
	{
		bool res{ false };
		for (PooledComponentVariant& component : pooledEntityBody_->components)
		{
			if (std::holds_alternative<PooledComponent<ComponentClass>>(component))
			{
				component = std::move(std::monostate{});
				res = true;
				break;
			}
		}
		return res;
	}

	template <std::size_t CAPACITY>
	template <Group group>
	constexpr bool EntitiesManager<CAPACITY>::Entity::isMemberOf() const noexcept
	{
		const auto grpsEnd{ std::cend(pooledEntityBody_->groups) };
		return std::find(std::cbegin(pooledEntityBody_->groups), grpsEnd, group) != grpsEnd;
	}

	template <std::size_t CAPACITY>
	template <Group group>
	constexpr bool EntitiesManager<CAPACITY>::Entity::enrollToGroup() noexcept
	{
		std::size_t firstEmpty{ groupsCount };
		for (std::size_t i{ 0U }; i != groupsCount; ++i)
		{
			if (pooledEntityBody_->groups[i] == group)
			{
				return false;
			}
			else if (firstEmpty == groupsCount &&
				pooledEntityBody_->groups[i] == Group::emptyVal)
			{
				firstEmpty = i;
			}
		}

		pooledEntityBody_->groups[firstEmpty] = group;
		return true;
	}

	template <std::size_t CAPACITY>
	template <Group group>
	constexpr bool EntitiesManager<CAPACITY>::Entity::dismissFromGroup() noexcept
	{
		const auto it{ std::ranges::find(pooledEntityBody_->groups, group) };
		if (it != std::end(pooledEntityBody_->groups))
		{
			*it = Group::emptyVal;
			return true;
		}
		else
		{
			return false;
		}
	}

}
#endif // !ENTITIES_MANAGER