
#ifndef COMPONENT_OBJECT_POOL
#define COMPONENT_OBJECT_POOL

#include "PhysicsComponent.hpp"
#include "LifetimeComponent.hpp"

#include <array>
#include <memory>
#include <mutex>


namespace ecs
{
    template <typename T, typename... Us>
    concept same_as_any_of = (std::same_as<T, Us> || ...);

    template<typename Component>
    concept has_valid_bool = requires(Component compo)
    {
        requires std::same_as<decltype(compo.valid), bool>;
    };

    template<typename Component>
    concept ComponentConcept = 
        std::is_trivially_copyable_v<Component> && 
        has_valid_bool<Component> &&
        same_as_any_of<Component, PhysicsComponent, LifetimeComponent>;


    template <ComponentConcept Component, std::size_t CAPACITY>
    class ComponentPool;

    template <ComponentConcept Component, std::size_t CAPACITY>
    class ComponentDeleter
    {
    public:
        // NOTE: ComponentDeleter's default ctor should generally never be used
        // as it allows for the following undefined behavior - 
        // PooledComponent<PhysicsComponent, 5U> physCompo; physCompo->xPos = 6;
        // I've defined it only for EntityBody.components_ which require a default ctor
        constexpr ComponentDeleter()
            : compoPool_{ nullptr }
        { }

        constexpr ComponentDeleter(ComponentPool<Component, CAPACITY>& compoPool)
            : compoPool_{ &compoPool }
        { }

        constexpr void operator()(Component* compo) const
        {
            // NOTE: The pool's lifetime must exceed that of its objects, 
            // otherwise it'll lead to undefined behavior

            compoPool_->release(compo);
        }

    private:
        ComponentPool<Component, CAPACITY>* compoPool_;
    };


    template <ComponentConcept Component, std::size_t CAPACITY>
    using PooledComponent = std::unique_ptr<Component, ComponentDeleter<Component, CAPACITY>>;


    class components_max_capacity_exception : public std::bad_alloc
    {
    public:
        char const* what() const throw() override
        {
            return "component pool reached max capacity.";
        }
    };


    template <ComponentConcept Component, std::size_t CAPACITY>
    class ComponentPool
    {
    public:
        constexpr ComponentPool() noexcept;

        [[nodiscard]] constexpr PooledComponent<Component, CAPACITY> request() noexcept(false);

        [[nodiscard]] consteval std::size_t capacity() const noexcept;

        [[nodiscard]] constexpr std::size_t size() const noexcept;

        [[nodiscard]] constexpr bool isFull() const noexcept;

        constexpr Component* begin() noexcept;

        constexpr Component* end() noexcept;

    private:
        friend class ComponentDeleter<Component, CAPACITY>;

        std::array<Component, CAPACITY> pool_;
        Component* poolStart_;
        std::array<std::size_t, CAPACITY> stack_;
        std::size_t stackTop_;
        std::size_t size_;
        //std::mutex mutex_;
        ComponentDeleter<Component, CAPACITY> compoDeleter_;

        constexpr void release(Component* compo) noexcept;
    };


    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr ComponentPool<Component, CAPACITY>::ComponentPool() noexcept
        : pool_{}
        , poolStart_{ reinterpret_cast<Component* const>(pool_.data()) }
        , stack_{}
        , stackTop_{ 0U }
        , size_{ 0U }
        //, mutex_{}
        , compoDeleter_{ *this }
    {
        for (std::size_t i{ 0U }; i != CAPACITY; ++i)
        {
            stack_[i] = i;
        }
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr PooledComponent<Component, CAPACITY> ComponentPool<Component, CAPACITY>::request() noexcept(false)
    {
        //std::lock_guard lock{ mutex_ };

        if (stackTop_ == CAPACITY) [[unlikely]]
        {
            throw components_max_capacity_exception{};
        }

        ++stackTop_;

        ++size_;

        Component* compo{ new (&pool_[stack_[stackTop_ - 1U]]) Component{} };
        compo->valid = true;

        return { compo, compoDeleter_ };
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr void ComponentPool<Component, CAPACITY>::release(Component* compo) noexcept
    {
        //std::lock_guard lock{ mutex_ };

        compo->valid = false;

        const std::size_t freedObjIdx{ static_cast<std::size_t>(compo - poolStart_) };

        --stackTop_;
        stack_[stackTop_] = freedObjIdx;

        --size_;
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    consteval std::size_t ComponentPool<Component, CAPACITY>::capacity() const noexcept
    {
        return CAPACITY;
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr std::size_t ComponentPool<Component, CAPACITY>::size() const noexcept
    {
        return size_;
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr bool ComponentPool<Component, CAPACITY>::isFull() const noexcept
    {
        return size_ == CAPACITY;
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr Component* ComponentPool<Component, CAPACITY>::begin() noexcept
    {
        return poolStart_;
    }

    template <ComponentConcept Component, std::size_t CAPACITY>
    constexpr Component* ComponentPool<Component, CAPACITY>::end() noexcept
    {
        return poolStart_ + CAPACITY;
    }
}

#endif // !COMPONENT_OBJECT_POOL