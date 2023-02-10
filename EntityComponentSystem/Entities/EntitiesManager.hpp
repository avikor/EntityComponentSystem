#ifndef ENTITIES_MANAGER
#define ENTITIES_MANAGER

#include "EntitiesPool.hpp"

#include <algorithm>


namespace ecs
{
	enum class Component : std::uint16_t
	{
		physics,
		lifetime
	};

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

			[[nodiscard]] bool hasComponent(Component compo) const noexcept;

			[[nodiscard]] PooledComponent<PhysicsComponent, CAPACITY>& getPhysicsComponent() noexcept;

			[[nodiscard]] PooledComponent<LifetimeComponent, CAPACITY>& getLifetimeComponent() noexcept;

			[[nodiscard]] bool addComponent(Component compo) noexcept;

			[[nodiscard]] bool removeComponent(Component compo) noexcept;

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
	bool EntitiesManager<CAPACITY>::Entity::hasComponent(Component compo) const noexcept
	{
		bool res{ false };
		if (compo == Component::physics)
		{
			res = pooledEntity_->physCompo_ != nullptr;
		}
		else if (compo == Component::lifetime)
		{
			res = pooledEntity_->lifetimeCompo_ != nullptr;
		}

		return res;
	}

	template <std::size_t CAPACITY>
	PooledComponent<PhysicsComponent, CAPACITY>& EntitiesManager<CAPACITY>::Entity::getPhysicsComponent() noexcept
	{
		return pooledEntity_->physCompo_;
	}

	template <std::size_t CAPACITY>
	PooledComponent<LifetimeComponent, CAPACITY>& EntitiesManager<CAPACITY>::Entity::getLifetimeComponent() noexcept
	{
		return pooledEntity_->lifetimeCompo_;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::Entity::addComponent(Component compo) noexcept
	{
		bool res{ false };

		if (compo == Component::physics && pooledEntity_->physCompo_ == nullptr)
		{
			pooledEntity_->physCompo_ = std::move(entitiesManager_.physicsComponentsPool_.request());
			res = true;
		}
		else if (compo == Component::lifetime && pooledEntity_->lifetimeCompo_ == nullptr)
		{
			pooledEntity_->lifetimeCompo_ = std::move(entitiesManager_.lifetimeComponentsPool_.request());
			res = true;
		}

		return res;
	}

	template <std::size_t CAPACITY>
	bool EntitiesManager<CAPACITY>::Entity::removeComponent(Component compo) noexcept
	{
		bool res{ false };

		if (compo == Component::physics && pooledEntity_->physCompo_ != nullptr)
		{
			pooledEntity_->physCompo_.reset();
			res = true;
		}
		else if (compo == Component::lifetime && pooledEntity_->lifetimeCompo_ != nullptr)
		{
			pooledEntity_->lifetimeCompo_.reset();
			res = true;
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