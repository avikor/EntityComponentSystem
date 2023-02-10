#include "MoveSystem.hpp"
#include "DecLifetimeSystem.hpp"
#include "DummySystem.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <future>


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

	REQUIRE_FALSE(ent1.hasComponent(ecs::Component::physics));
	REQUIRE_FALSE(ent1.hasComponent(ecs::Component::lifetime));

	
	REQUIRE_FALSE(ent1.removeComponent(ecs::Component::physics));
	REQUIRE_FALSE(ent1.removeComponent(ecs::Component::lifetime));

	REQUIRE(ent1.addComponent(ecs::Component::physics));
	REQUIRE_FALSE(ent1.addComponent(ecs::Component::physics));

	REQUIRE(ent1.hasComponent(ecs::Component::physics));

	ecs::PooledComponent<ecs::PhysicsComponent, 2U>& physCompo = ent1.getPhysicsComponent();;
	
	REQUIRE(physCompo->valid == true);
	REQUIRE(physCompo->xPos == 0.0f);
	REQUIRE(physCompo->yPos == 0.0f);
	REQUIRE(physCompo->xVelocity == 0.0f);
	REQUIRE(physCompo->yVelocity == 0.0f);

	physCompo->xPos = 10.1f;
	REQUIRE(physCompo->xPos == 10.1f);


	REQUIRE(ent1.removeComponent(ecs::Component::physics));
	REQUIRE_FALSE(ent1.hasComponent(ecs::Component::physics));

	REQUIRE(ent1.addComponent(ecs::Component::lifetime));
	REQUIRE_FALSE(ent1.addComponent(ecs::Component::lifetime));
	REQUIRE(ent1.hasComponent(ecs::Component::lifetime));

	ecs::PooledComponent<ecs::LifetimeComponent, 2U>& lifetimeCompo = ent1.getLifetimeComponent();;
	lifetimeCompo->lifetime = 17;

	REQUIRE(ent1.removeComponent(ecs::Component::lifetime));
	REQUIRE_FALSE(ent1.removeComponent(ecs::Component::lifetime));
	REQUIRE_FALSE(ent1.hasComponent(ecs::Component::lifetime));
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

	REQUIRE(ent1.addComponent(ecs::Component::physics));
	REQUIRE(ent2.addComponent(ecs::Component::physics));
	REQUIRE(ent3.addComponent(ecs::Component::physics));

	ecs::EntitiesManager<8U>::Entity ent4 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent5 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent6 = entitiesManager.requestEntity();

	REQUIRE(ent4.addComponent(ecs::Component::lifetime));
	REQUIRE(ent5.addComponent(ecs::Component::lifetime));
	REQUIRE(ent6.addComponent(ecs::Component::lifetime));

	ecs::EntitiesManager<8U>::Entity ent7 = entitiesManager.requestEntity();
	ecs::EntitiesManager<8U>::Entity ent8 = entitiesManager.requestEntity();

	REQUIRE(ent7.addComponent(ecs::Component::physics));
	REQUIRE(ent7.addComponent(ecs::Component::lifetime));
	REQUIRE(ent8.addComponent(ecs::Component::physics));
	REQUIRE(ent8.addComponent(ecs::Component::lifetime));
	
	REQUIRE(ent7.enrollToGroup(ecs::Group::dummy_group));
	REQUIRE(ent8.enrollToGroup(ecs::Group::dummy_group));

	std::future<void> fuMove = std::async(std::launch::async, ecs::move_system<8U>, std::ref(entitiesManager));
	std::future<void> fuDec = std::async(std::launch::async, ecs::decrease_lifetime_system<8U>, std::ref(entitiesManager));
	std::future<void> fuDummy = std::async(std::launch::async, ecs::dummy_system<8U>, std::ref(entitiesManager));

	fuMove.get();
	fuDec.get();
	fuDummy.get();
}