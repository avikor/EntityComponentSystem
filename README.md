## EntityComponentSystem
This repository contains an [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) (ECS) implemented using C++20.
- A <em>Component</em> is a [Plain Old Data type (POD)](https://en.wikipedia.org/wiki/Passive_data_structure). In this implementation it's a [Trivially Copyable type](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable).
- An <em>Entity</em> is a container of components. These components can be added or removed from the entity. In this implementation an entity is essentially a [std::array](https://en.cppreference.com/w/cpp/container/array) of [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr) to [std::variant](https://en.cppreference.com/w/cpp/utility/variant).
- A <em>System</em> is a function who uses entities' components to perform a computation.

An entity component system "facilitates code reusability by separating the data from the behavior" ([source](https://www.simplilearn.com/entity-component-system-introductory-guide-article)). Components and entities handle the data. Systems handle the behavior.<br><br>Since all entities are of the same class, an entity may enroll in one or more groups in order to differentiate its behavior from other entities.<br>These groups can be used as an alternative to classes, since the user can write a system which handles only entities who are members of a specific group, thus differentiating their behavior from other entities.<br><br>Additionally, this implementation doesn't use inheritance so there is no overhead due [dynamic dispatch](https://en.wikipedia.org/wiki/Dynamic_dispatch#C++_implementation). <br><br>This entity component system was built with the following ideas in mind:<br>
1. [Principle of locality](https://en.wikipedia.org/wiki/Locality_of_reference).
2. [Object pool design pattern](https://en.wikipedia.org/wiki/Object_pool_pattern).
3. [Entity Component System architectural pattern](https://en.wikipedia.org/wiki/Entity_component_system).
4. [Multithreading](https://en.wikipedia.org/wiki/Multithreading_(computer_architecture)).

In short: It couples entity-component-system-architectural-pattern with object-pool-design-pattern to fully leverage principle-of-locality-based-optimizations performed by multiple threads.
It does so by pooling both components and entities in object pools, and by executing the systems asynchronously.
<br>Components and entities are allocated at compile time using their respective pools. <br>Each component type has its own pool, and all entities are allocated in a single entities pool. <br>Since an entity is essentially a std::array of std::unique_ptr to std::variant, iterating over an entity's components isn't as fast as iterating directly over all components of a specific type, since they are stored by their pool contiguously in memory.<br>The user of this repository is highly advised to design its components in a way such that when a system uses a component to perform its computation, it has all the data it needs in that component, rather than having to query for another component of that entity.<br>A good rule of thumb is that if a system needs two components to perform its computation, it's probably better to combine the two components into a single component.<br><br>Some toy examples are present at 'EntityComponentSystem/ecsTests.cpp'.<br>NOTE: this implementation is not entirely thread-safe, as the Entity class is not protected by a mutex.<br>The allocation and deallocation of components and entities is thread-safe however. 