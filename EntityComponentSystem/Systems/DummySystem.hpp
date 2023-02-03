#ifndef DUMMY_SYSTEM
#define DUMMY_SYSTEM

#include "EntitiesManager.hpp"

namespace ecs
{
	template <std::size_t CAPACITY>
	constexpr void dummy_system(EntitiesManager<CAPACITY>& entitiesManager)
	{
		for (EntityBody<CAPACITY>& entBody : entitiesManager.entitiesPool_)
		{
			const auto grpsEnd{ std::cend(entBody.groups_) };
			if (std::find(std::cbegin(entBody.groups_), grpsEnd, Group::dummy_group) != grpsEnd)
			{
				// do somthing
			}
		}
	}
}

#endif // !DUMMY_SYSTEM
