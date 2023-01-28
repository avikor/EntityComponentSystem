#ifndef ENTITIES_OBJECT_POOL
#define ENTITIES_OBJECT_POOL

#include "ComponentObjectPool.hpp"

#include <variant>


namespace ecs
{
    using EntityId = unsigned long;

    using PooledComponentVariant =
        std::variant<std::monostate,
        PooledComponent<PhysicsComponent>,
        PooledComponent<LifetimeComponent>>;


    enum class Group : std::uint32_t
    {
        emptyVal,

        // some examples for possible groups
        // client should define groups as it wishes
        movers,
        organisms,
        dummy_group,
        // examples' end

        count
    };

    static constexpr std::uint32_t componentClassesCount{ std::variant_size_v<PooledComponentVariant> - 1U };

    static constexpr std::uint32_t groupsCount{ static_cast<std::underlying_type_t<Group>>(Group::count) - 1U };

    struct EntityBody
    {
        EntityId id_{ 0U };
        std::array<PooledComponentVariant, componentClassesCount> components_{};
        std::array<Group, groupsCount> groups_{};
    };


    using PooledEntityBody = std::unique_ptr<EntityBody, std::function<void(EntityBody*)>>;


    class entities_max_capacity_exception : public std::bad_alloc
    {
    public:
        char const* what() const throw() override
        {
            return "entities pool reached max capacity.";
        }
    };


    template <std::size_t CAPACITY>
    class EntitiesObjectPool
    {
    public:
        EntitiesObjectPool() noexcept;

        [[nodiscard]] constexpr PooledEntityBody request() noexcept(false);

        [[nodiscard]] consteval std::size_t capacity() const noexcept;

        [[nodiscard]] constexpr std::size_t size() const noexcept;

        [[nodiscard]] constexpr bool isFull() const noexcept;

        EntityBody* begin() noexcept;

        EntityBody* end() noexcept;

    private:
        std::array<EntityBody, CAPACITY> pool_;
        EntityBody* const poolStart_;
        std::array<std::size_t, CAPACITY> stack_;
        std::size_t stackTop_;
        std::size_t size_;
        std::mutex mutex_;
    };


    template <std::size_t CAPACITY>
    EntitiesObjectPool<CAPACITY>::EntitiesObjectPool() noexcept
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

    template <std::size_t CAPACITY>
    constexpr PooledEntityBody EntitiesObjectPool<CAPACITY>::request() noexcept(false)
    {
        std::lock_guard lock{ mutex_ };

        if (stackTop_ == CAPACITY) [[unlikely]]
        {
            throw entities_max_capacity_exception{};
        }

        ++stackTop_;

        ++size_;

        return { new (&pool_[stack_[stackTop_ - 1U]]) EntityBody{}, [this](EntityBody* entBody)
            {
                // NOTE: The pool's lifetime must exceed that of its objects, 
                // otherwise it'll lead to undefined behavior

                std::lock_guard lock{ mutex_ };

                const std::size_t freedObjIdx{ static_cast<std::size_t>(entBody - poolStart_) };

                for (PooledComponentVariant& component : entBody->components_)
                {
                    component = std::move(std::monostate{});
                }
                entBody->~EntityBody();

                --stackTop_;
                stack_[stackTop_] = freedObjIdx;

                --size_;
            }
        };
    }

    template <std::size_t CAPACITY>
    consteval std::size_t EntitiesObjectPool<CAPACITY>::capacity() const noexcept
    {
        return CAPACITY;
    }

    template <std::size_t CAPACITY>
    constexpr std::size_t EntitiesObjectPool<CAPACITY>::size() const noexcept
    {
        return size_;
    }

    template <std::size_t CAPACITY>
    constexpr bool EntitiesObjectPool<CAPACITY>::isFull() const noexcept
    {
        return size_ == CAPACITY;
    }

    template <std::size_t CAPACITY>
    EntityBody* EntitiesObjectPool<CAPACITY>::begin() noexcept
    {
        return pool_.data();
    }

    template <std::size_t CAPACITY>
    EntityBody* EntitiesObjectPool<CAPACITY>::end() noexcept
    {
        return pool_.data() + CAPACITY;
    }
}


#endif // !ENTITIES_OBJECT_POOL
