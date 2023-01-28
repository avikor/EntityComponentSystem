#ifndef PHYSICS_COMPONENT
#define PHYSICS_COMPONENT

namespace ecs
{
	struct PhysicsComponent
	{
		bool valid{ false };
		float xPos{ 0.0f };
		float yPos{ 0.0f };
		float xVelocity{ 0.0f };
		float yVelocity{ 0.0f };
	};
}

#endif // !PHYSICS_COMPONENT