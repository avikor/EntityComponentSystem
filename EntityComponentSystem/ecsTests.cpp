#include "MoveSystem.hpp"
#include "DecLifetimeSystem.hpp"
#include "DummySystem.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <future>

template<std::size_t CAPACITY>
struct PhysicsVisitor
{
	void operator()(std::monostate empty) const
	{
		REQUIRE(false); // never meant to reach here
	}

	void operator()(ecs::PooledComponent<ecs::PhysicsComponent, CAPACITY>& physComp) const
	{
		physComp->xPos = 5.34f;
		physComp->yPos = 1.4f;
		physComp->xVelocity = 2.0f;
		physComp->yVelocity = 0.5f;
	}

	void operator()(ecs::PooledComponent<ecs::LifetimeComponent, CAPACITY>& lifetimeComp) const
	{
		REQUIRE(false); // never meant to reach here
	}
};


TEST_CASE("EntitiesManager::isFull")
{
	ecs::EntitiesManager<2U> entitiesManager{};

	{
		ecs::EntitiesManager<2U>::Entity ent1 = entitiesManager.requestEntity();
		REQUIRE_FALSE(entitiesManager.isFull());
		REQUIRE(ent1.getId() == 0U);
	}
	
	{
		ecs::EntitiesManager<2U>::Entity ent1 = entitiesManager.requestEntity();
		REQUIRE_FALSE(entitiesManager.isFull());
		REQUIRE(ent1.getId() == 1U);

		ecs::EntitiesManager<2U>::Entity ent2 = entitiesManager.requestEntity();
		REQUIRE(entitiesManager.isFull());
		REQUIRE(ent2.getId() == 2U);

		REQUIRE_THROWS_AS(entitiesManager.requestEntity(), ecs::entities_max_capacity_exception);
	}

	ecs::EntitiesManager<2U>::Entity ent1 = entitiesManager.requestEntity();
	REQUIRE_FALSE(entitiesManager.isFull());
	REQUIRE(ent1.getId() == 3U);

	ecs::EntitiesManager<2U>::Entity ent2 = entitiesManager.requestEntity();
	REQUIRE(entitiesManager.isFull());
	REQUIRE(ent2.getId() == 4U);

	REQUIRE_THROWS_AS(entitiesManager.requestEntity(), ecs::entities_max_capacity_exception);
}

TEST_CASE("Entity::components")
{
	// hasComponent, getComponent, addComponent, removeComponent
	ecs::EntitiesManager<2U> entitiesManager{};

	ecs::EntitiesManager<2U>::Entity ent1 = entitiesManager.requestEntity();

	REQUIRE_FALSE(ent1.hasComponent<ecs::PhysicsComponent>());
	REQUIRE_FALSE(ent1.hasComponent<ecs::LifetimeComponent>());

	ecs::PooledVariant<2U>& wantedPhysCompoVar = ent1.getComponent<ecs::PhysicsComponent>();
	REQUIRE(std::holds_alternative<std::monostate>(wantedPhysCompoVar));
	
	ecs::PooledVariant<2U>& wantedLifetimeCompoVar = ent1.getComponent<ecs::LifetimeComponent>();
	REQUIRE(std::holds_alternative<std::monostate>(wantedLifetimeCompoVar));

	REQUIRE_FALSE(ent1.removeComponent<ecs::PhysicsComponent>());
	REQUIRE_FALSE(ent1.removeComponent<ecs::LifetimeComponent>());

	REQUIRE(ent1.addComponent<ecs::PhysicsComponent>());
	REQUIRE_FALSE(ent1.addComponent<ecs::PhysicsComponent>());

	REQUIRE(ent1.hasComponent<ecs::PhysicsComponent>());

	ecs::PooledVariant<2U>& physCompoVar = ent1.getComponent<ecs::PhysicsComponent>();
	
	REQUIRE(std::holds_alternative<ecs::PooledComponent<ecs::PhysicsComponent, 2U>>(physCompoVar));

	auto& physCompo = std::get<ecs::PooledComponent<ecs::PhysicsComponent, 2U>>(physCompoVar);

	REQUIRE(physCompo->valid == true);
	REQUIRE(physCompo->xPos == 0.0f);
	REQUIRE(physCompo->yPos == 0.0f);
	REQUIRE(physCompo->xVelocity == 0.0f);
	REQUIRE(physCompo->yVelocity == 0.0f);

	physCompo->xPos = 10.1f;
	REQUIRE(physCompo->xPos == 10.1f);

	std::visit(PhysicsVisitor<2U>{}, physCompoVar);

	REQUIRE(physCompo->valid == true);
	REQUIRE(physCompo->xPos == 5.34f);
	REQUIRE(physCompo->yPos == 1.4f);
	REQUIRE(physCompo->xVelocity == 2.0f);
	REQUIRE(physCompo->yVelocity == 0.5f);

	REQUIRE(ent1.removeComponent<ecs::PhysicsComponent>());
	REQUIRE_FALSE(ent1.hasComponent<ecs::PhysicsComponent>());
	ecs::PooledVariant<2U>& wantedPhysCompoVar2 = ent1.getComponent<ecs::PhysicsComponent>();
	REQUIRE(std::holds_alternative<std::monostate>(wantedPhysCompoVar2));
}

TEST_CASE("Entity::groups")
{
	// isMemberOf, enrollToGroup, dismissFromGroup
	ecs::EntitiesManager<2U> entitiesManager{};

	ecs::EntitiesManager<2U>::Entity ent1 = entitiesManager.requestEntity();

	REQUIRE_FALSE(ent1.isMemberOf(ecs::Group::movers));
	REQUIRE_FALSE(ent1.isMemberOf(ecs::Group::organisms));

	REQUIRE_FALSE(ent1.dismissFromGroup(ecs::Group::movers));
	REQUIRE_FALSE(ent1.dismissFromGroup(ecs::Group::organisms));

	REQUIRE(ent1.enrollToGroup(ecs::Group::movers));
	REQUIRE_FALSE(ent1.enrollToGroup(ecs::Group::movers));

	REQUIRE(ent1.isMemberOf(ecs::Group::movers));
	REQUIRE_FALSE(ent1.isMemberOf(ecs::Group::organisms));

	REQUIRE(ent1.dismissFromGroup(ecs::Group::movers));
	REQUIRE_FALSE(ent1.isMemberOf(ecs::Group::movers));
	REQUIRE_FALSE(ent1.dismissFromGroup(ecs::Group::movers));

	REQUIRE(ent1.enrollToGroup(ecs::Group::movers));
	REQUIRE(ent1.enrollToGroup(ecs::Group::organisms));
}

TEST_CASE("systems")
{
	ecs::EntitiesManager<8U> entitiesManager{};

	ecs::EntitiesManager<8U>::Entity ent1 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent2 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent3 = entitiesManager.requestEntity();

	REQUIRE(ent1.addComponent<ecs::PhysicsComponent>());
	REQUIRE(ent2.addComponent<ecs::PhysicsComponent>());
	REQUIRE(ent3.addComponent<ecs::PhysicsComponent>());

	ecs::EntitiesManager<8U>::Entity ent4 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent5 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent6 = entitiesManager.requestEntity();

	REQUIRE(ent4.addComponent<ecs::LifetimeComponent>());
	REQUIRE(ent5.addComponent<ecs::LifetimeComponent>());
	REQUIRE(ent6.addComponent<ecs::LifetimeComponent>());

	ecs::EntitiesManager<8U>::Entity ent7 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent8 = entitiesManager.requestEntity();

	REQUIRE(ent7.addComponent<ecs::PhysicsComponent>());
	REQUIRE(ent7.addComponent<ecs::LifetimeComponent>());
	REQUIRE(ent8.addComponent<ecs::PhysicsComponent>());
	REQUIRE(ent8.addComponent<ecs::LifetimeComponent>());
	
	REQUIRE(ent7.enrollToGroup(ecs::Group::dummy_group));
	REQUIRE(ent8.enrollToGroup(ecs::Group::dummy_group));

	std::future<void> fuMove = std::async(std::launch::async, ecs::move_system<8U>, std::ref(entitiesManager));
	std::future<void> fuDec = std::async(std::launch::async, ecs::decrease_lifetime_system<8U>, std::ref(entitiesManager));
	std::future<void> fuDummy = std::async(std::launch::async, ecs::dummy_system<8U>, std::ref(entitiesManager));

	fuMove.get();
	fuDec.get();
	fuDummy.get();
}