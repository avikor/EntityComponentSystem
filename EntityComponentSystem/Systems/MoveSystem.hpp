#ifndef MOVE_SYSTEM
#define MOVE_SYSTEM

#include "EntitiesManager.hpp"

namespace ecs
{
	template <std::size_t CAPACITY>
	constexpr void move_system(EntitiesManager<CAPACITY>& entitiesManager)
	{
		for (PhysicsComponent& physComp : entitiesManager.physicsComponentsPool_)
		{
			if (physComp.valid)
			{
				physComp.xPos += physComp.xVelocity;
				physComp.yPos += physComp.yVelocity;
			}
		}
	}
}

#endif // !MOVE_SYSTEM
