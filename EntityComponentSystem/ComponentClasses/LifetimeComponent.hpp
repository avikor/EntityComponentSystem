#ifndef LIFETIME_COMPONENT
#define LIFETIME_COMPONENT

#include <cstdint>

namespace ecs
{
	struct LifetimeComponent
	{
		bool valid{ false };
		uint32_t lifetime{ 0 };
	};
}

#endif // !LIFETIME_COMPONENT
