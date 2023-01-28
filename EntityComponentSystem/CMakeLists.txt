add_subdirectory("ComponentClasses")
add_subdirectory("Entities")
add_subdirectory("Systems")
add_subdirectory("Pools")

add_executable (EntityComponentSystem	"ComponentClasses/PhysicsComponent.hpp"										
										"ComponentClasses/LifetimeComponent.hpp"
										"Pools/ComponentObjectPool.hpp"
										"Pools/EntitiesObjectPool.hpp"
										"Entities/EntitiesManager.hpp" 
										"Systems/DecLifetimeSystem.hpp"
										"Systems/MoveSystem.hpp"
										"Systems/DummySystem.hpp"
										"catch.hpp"
										"ecsTests.cpp")


target_include_directories(EntityComponentSystem PRIVATE "ComponentClasses" "Entities" "Systems" "Pools")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET EntityComponentSystem PROPERTY CXX_STANDARD 20)
endif()