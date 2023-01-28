#ifndef COMPONENT_OBJECT_POOL
#define COMPONENT_OBJECT_POOL

#include "PhysicsComponent.hpp"
#include "LifetimeComponent.hpp"

#include <array>
#include <memory>
#include <functional>
#include <mutex>


namespace ecs
{
    template<typename Component>
    concept hasValid = requires(Component compo)
    {
        compo.valid;
        requires std::same_as<decltype(compo.valid), bool>;
    };

    template<typename Component>
    concept ComponentClassConcept = std::is_trivially_copyable_v<Component> && hasValid<Component> &&
        (std::same_as<Component, PhysicsComponent> ||
        std::same_as<Component, LifetimeComponent>);


    template <ComponentClassConcept Component>
    using PooledComponent = std::unique_ptr<Component, std::function<void(Component*)>>;


    class components_max_capacity_exception : public std::bad_alloc
    {
    public:
        char const* what() const throw() override
        {
            return "component pool reached max capacity.";
        }
    };


    template <ComponentClassConcept Component, std::size_t CAPACITY>
    class ComponentObjectPool
    {
    public:
        ComponentObjectPool() noexcept;

        [[nodiscard]] constexpr PooledComponent<Component> request() noexcept(false);

        [[nodiscard]] consteval std::size_t capacity() const noexcept;

        [[nodiscard]] constexpr std::size_t size() const noexcept;

        [[nodiscard]] constexpr bool isFull() const noexcept;

        Component* begin() noexcept;

        Component* end() noexcept;

    private:
        std::array<Component, CAPACITY> pool_;
        Component* const poolStart_;
        std::array<std::size_t, CAPACITY> stack_;
        std::size_t stackTop_;
        std::size_t size_;
        std::mutex mutex_;
    };


    template <ComponentClassConcept Component, std::size_t CAPACITY>
    ComponentObjectPool<Component, CAPACITY>::ComponentObjectPool() noexcept
        : pool_{}
        , poolStart_{ pool_.data() }
        , stack_{}
        , stackTop_{ 0U }
        , size_{ 0U }
        , mutex_{}
    {
        for (std::size_t i{ 0U }; i != CAPACITY; ++i)
        {
            stack_[i] = i;
        }
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    constexpr PooledComponent<Component> ComponentObjectPool<Component, CAPACITY>::request() noexcept(false)
    {
        std::lock_guard lock{ mutex_ };

        if (stackTop_ == CAPACITY) [[unlikely]]
        {
            throw components_max_capacity_exception{};
        }

        ++stackTop_;

        ++size_;

        Component* compo{ new (&pool_[stack_[stackTop_ - 1U]]) Component{} };
        compo->valid = true;

        return { compo, [this](Component* compo)
            {
                // NOTE: The pool's lifetime must exceed that of its objects, 
                // otherwise it'll lead to undefined behavior

                std::lock_guard lock{ mutex_ };

                const std::size_t freedObjIdx{ static_cast<std::size_t>(compo - poolStart_) };

                compo->valid = false;

                --stackTop_;
                stack_[stackTop_] = freedObjIdx;

                --size_;
            }
        };
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    consteval std::size_t ComponentObjectPool<Component, CAPACITY>::capacity() const noexcept
    {
        return CAPACITY;
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    constexpr std::size_t ComponentObjectPool<Component, CAPACITY>::size() const noexcept
    {
        return size_;
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    constexpr bool ComponentObjectPool<Component, CAPACITY>::isFull() const noexcept
    {
        return size_ == CAPACITY;
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    Component* ComponentObjectPool<Component, CAPACITY>::begin() noexcept
    {
        return pool_.data();
    }

    template <ComponentClassConcept Component, std::size_t CAPACITY>
    Component* ComponentObjectPool<Component, CAPACITY>::end() noexcept
    {
        return pool_.data() + CAPACITY;
    }
}


#endif // !COMPONENT_OBJECT_POOL