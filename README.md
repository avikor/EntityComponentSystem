## EntityComponentSystem
This repository contains an [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) (ECS) implemented using C++20.
- A <em>Component</em> is a [Plain Old Data type (POD)](https://en.wikipedia.org/wiki/Passive_data_structure). In our implementation they are [Trivially Copyable types](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable).
- An <em>Entity</em> is a container of components. These components can be added or removed from the entity. In implementation they are more or less a [std::array](https://en.cppreference.com/w/cpp/container/array) of [std::variant](https://en.cppreference.com/w/cpp/utility/variant).
- A <em>System</em> is a function who uses entities' components to perform a computation.

An entity component system "facilitates code reusability by separating the data from the behavior" ([source](https://www.simplilearn.com/entity-component-system-introductory-guide-article)). Components and entities handle the data. Systems handle the behavior.<br><br>This ECS was built with the following ideas in mind:<br>
1. [Principle of locality](https://en.wikipedia.org/wiki/Locality_of_reference).
2. [Object pool design pattern](https://en.wikipedia.org/wiki/Object_pool_pattern).
3. [Entity Component System architectural pattern](https://en.wikipedia.org/wiki/Entity_component_system).
4. [Multithreading (computer architecture)](https://en.wikipedia.org/wiki/Multithreading_(computer_architecture)).

In short: It couples entity-component-system-architectural-pattern with object-pool-design-pattern to fully leverage principle-of-locality-based-optimizations performed by multiple threads.<br>It does so by pooling both components and entities in object pools, and by executing the systems asynchronously.<br>The user of this repository is highly advised to design its components in a way such that, when a system uses the component it has all the data it needs in that component, rather than having to query for another component in order to perform its computation.<br>A good rule of thumb is that if a system needs two components to perform its computation, it's probably more correct to combine the two components into a single component.