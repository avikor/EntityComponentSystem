#ifndef ENTITIES_MANAGER
#define ENTITIES_MANAGER

#include "EntitiesPool.hpp"

#include <typeinfo>
#include <algorithm>


namespace ecs
{
	template <std::size_t CAPACITY>
	class EntitiesManager
	{
	public:
		class Entity;

		[[nodiscard]] Entity requestEntity() noexcept(false);

		[[nodiscard]] bool isFull() const noexcept;

		class Entity
		{
		public:
			[[nodiscard]] EntityId getId() const noexcept;

			template <ComponentConcept Component>
			[[nodiscard]] bool hasComponent() const noexcept;

			template <ComponentConcept Component>
			[[nodiscard]] PooledVariant<CAPACITY>& getComponent() noexcept;

			template <ComponentConcept Component>
			[[nodiscard]] bool addComponent() noexcept;

			template <ComponentConcept Component>
			[[nodiscard]] bool removeComponent() noexcept;

			[[nodiscard]] bool isMemberOf(Group group) const noexcept;

			[[nodiscard]] bool enrollToGroup(Group group) noexcept;

			[[nodiscard]] bool dismissFromGroup(Group group) noexcept;


		private:
			friend EntitiesManager<CAPACITY>;

			EntitiesManager& entitiesManager_;
			PooledEntityBody<CAPACITY> pooledEntity_;

			explicit Entity(EntitiesManager& entitiesManager);
		};


	private:
		// systems
		friend void move_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);
		friend void decrease_lifetime_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);
		friend void dummy_system<CAPACITY>(EntitiesManager<CAPACITY>& entitiesManager);

		static EntityId nextId_s;

		

		ComponentPool<PhysicsComponent, CAPACITY> physicsComponentsPool_;
		ComponentPool<LifetimeComponent, CAPACITY> lifetimeComponentsPool_;

		EntitiesPool<CAPACITY> entitiesPool_;

		std::mutex mu_;
	};


	//////// EntitiesManager definitions //////// 
	template <std::size_t CAPACITY>
	EntityId EntitiesManager<CAPACITY>::nextId_s{ 0U };

	template <std::size_t CAPACITY>
	EntitiesManager<CAPACITY>::Entity EntitiesManager<CAPACITY>::requestEntity() noexcept(false)
	{
		std::lock_guard lock{ mu_ };

		Entity ent{ *this };
		ent.pooledEntity_->id_ = EntitiesManager<CAPACITY>::nextId_s++;
		return ent;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::isFull() const noexcept
	{
		return entitiesPool_.isFull();
	}

	//////// Entity definitions //////// 
	template <std::size_t CAPACITY>
	EntityId EntitiesManager<CAPACITY>::Entity::getId() const noexcept
	{
		return pooledEntity_->id_;
	}

	template <std::size_t CAPACITY>
	EntitiesManager<CAPACITY>::Entity::Entity(EntitiesManager& entitiesManager)
		: entitiesManager_{ entitiesManager }
		, pooledEntity_{ entitiesManager_.entitiesPool_.request() }
	{ }

	template <std::size_t CAPACITY>
	template <ComponentConcept Component>
	bool EntitiesManager<CAPACITY>::Entity::hasComponent() const noexcept
	{
		for (const PooledVariant<CAPACITY>& component : pooledEntity_->components_)
		{
			if (std::holds_alternative<PooledComponent<Component, CAPACITY>>(component))
			{
				return true;
			}
		}

		return false;
	}

	template <std::size_t CAPACITY>
	template <ComponentConcept Component>
	PooledVariant<CAPACITY>& EntitiesManager<CAPACITY>::Entity::getComponent() noexcept
	{
		// notice that due to EntityBody's definition if the wanted component isn't contained,
		// then its alternative is a std::monostate

		std::size_t firstEmpty{ componentClassesCount<CAPACITY> };
		for (std::size_t i{ 0U }; i != componentClassesCount<CAPACITY>; ++i)
		{
			if (std::holds_alternative<PooledComponent<Component, CAPACITY>>(pooledEntity_->components_[i]))
			{
				return pooledEntity_->components_[i];
			}
			else if (firstEmpty == componentClassesCount<CAPACITY> &&
				std::holds_alternative<std::monostate>(pooledEntity_->components_[i]))
			{
				firstEmpty = i;
			}
		}

		return pooledEntity_->components_[firstEmpty];
	}

	template <std::size_t CAPACITY>
	template <ComponentConcept Component>
	bool EntitiesManager<CAPACITY>::Entity::addComponent() noexcept
	{
		std::size_t firstEmpty{ componentClassesCount<CAPACITY> };
		for (std::size_t i{ 0U }; i != componentClassesCount<CAPACITY>; ++i)
		{
			if (std::holds_alternative<PooledComponent<Component, CAPACITY>>(pooledEntity_->components_[i]))
			{
				return false;
			}
			else if (firstEmpty == componentClassesCount<CAPACITY> &&
				std::holds_alternative<std::monostate>(pooledEntity_->components_[i]))
			{
				firstEmpty = i;
			}
		}

		// if we've reached here than firstEmpty != componentClassesCount<CAPACITY>.
		// proof by negation: assume firstEmpty == componentClassesCount<CAPACITY> then
		// none of pooledEntity_->components_ is std::monostate, hence some variant 
		// is already the component the client wished to add, so we've returned false

		// notice Component is never polymorphic, therefore "typeid(Component)" 
		// is resolved at compile time, without additional runtime overhead, see "Notes" at -
		// https://en.cppreference.com/w/cpp/language/typeid

		if (typeid(Component) == typeid(PhysicsComponent))
		{
			pooledEntity_->components_[firstEmpty] = std::move(entitiesManager_.physicsComponentsPool_.request());
		}
		else if (typeid(Component) == typeid(LifetimeComponent))
		{
			pooledEntity_->components_[firstEmpty] = std::move(entitiesManager_.lifetimeComponentsPool_.request());
		}
		else
		{
			return false;
		}

		return true;
	}

	template <std::size_t CAPACITY>
	template <ComponentConcept Component>
	bool EntitiesManager<CAPACITY>::Entity::removeComponent() noexcept
	{
		bool res{ false };
		for (PooledVariant<CAPACITY>& component : pooledEntity_->components_)
		{
			if (std::holds_alternative<PooledComponent<Component, CAPACITY>>(component))
			{
				component = std::move(std::monostate{});
				res = true;
				break;
			}
		}
		return res;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::Entity::isMemberOf(Group group) const noexcept
	{
		const auto grpsEnd{ std::cend(pooledEntity_->groups_) };
		return std::find(std::cbegin(pooledEntity_->groups_), grpsEnd, group) != grpsEnd;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::Entity::enrollToGroup(Group group) noexcept
	{
		std::size_t firstEmpty{ groupsCount };
		for (std::size_t i{ 0U }; i != groupsCount; ++i)
		{
			if (pooledEntity_->groups_[i] == group)
			{
				return false;
			}
			else if (firstEmpty == groupsCount &&
				pooledEntity_->groups_[i] == Group::emptyVal)
			{
				firstEmpty = i;
			}
		}

		pooledEntity_->groups_[firstEmpty] = group;
		return true;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::Entity::dismissFromGroup(Group group) noexcept
	{
		const auto it{ std::ranges::find(pooledEntity_->groups_, group) };
		if (it != std::end(pooledEntity_->groups_))
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