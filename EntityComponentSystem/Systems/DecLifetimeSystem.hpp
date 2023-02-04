#ifndef DECREASE_LIFETIME_SYSTEM
#define DECREASE_LIFETIME_SYSTEM

#include "EntitiesManager.hpp"

namespace ecs
{
	template <std::size_t CAPACITY>
	void decrease_lifetime_system(EntitiesManager<CAPACITY>& entitiesManager)
	{
		for (LifetimeComponent& lifetimeComp : entitiesManager.lifetimeComponentsPool_)
		{
			if (lifetimeComp.valid)
			{
				--lifetimeComp.lifetime;
				if (lifetimeComp.lifetime == 0U)
				{
					// do something
				}
			}
		}
	}
}

#endif // !DECREASE_LIFETIME_SYSTEM
