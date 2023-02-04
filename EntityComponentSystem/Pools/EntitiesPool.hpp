#ifndef ENTITIES_OBJECT_POOL
#define ENTITIES_OBJECT_POOL

#include "ComponentPool.hpp"

#include <variant>


namespace ecs
{
    using EntityId = unsigned long;

    template<std::size_t CAPACITY>
    using PooledVariant =
        std::variant<std::monostate,
        PooledComponent<PhysicsComponent, CAPACITY>,
        PooledComponent<LifetimeComponent, CAPACITY>>;


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

    template<std::size_t CAPACITY>
    static constexpr std::uint32_t componentClassesCount{ std::variant_size_v<PooledVariant<CAPACITY>> - 1U };

    static constexpr std::uint32_t groupsCount{ static_cast<std::underlying_type_t<Group>>(Group::count) - 1U };

    template<std::size_t CAPACITY>
    struct EntityBody
    {
        EntityId id_{ 0U };
        std::array<PooledVariant<CAPACITY>, componentClassesCount<CAPACITY>> components_{};
        std::array<Group, groupsCount> groups_{};
    };


    template <std::size_t CAPACITY>
    class EntitiesPool;

    template <std::size_t CAPACITY>
    class EntityDeleter
    {
    public:
        EntityDeleter(EntitiesPool<CAPACITY>& entitiesPool)
            : entitiesPool_{ entitiesPool }
        { }

        void operator()(EntityBody<CAPACITY>* entBody) const
        {
            // NOTE: The pool's lifetime must exceed that of its objects, 
            // otherwise it'll lead to undefined behavior

            entitiesPool_.release(entBody);
        }

    private:
        EntitiesPool<CAPACITY>& entitiesPool_;
    };


    template<std::size_t CAPACITY>
    using PooledEntityBody = std::unique_ptr<EntityBody<CAPACITY>, const EntityDeleter<CAPACITY>&>;


    class entities_max_capacity_exception : public std::bad_alloc
    {
    public:
        char const* what() const throw() override
        {
            return "entities pool reached max capacity.";
        }
    };


    template <std::size_t CAPACITY>
    class EntitiesPool
    {
    public:
        EntitiesPool() noexcept;

        [[nodiscard]] PooledEntityBody<CAPACITY> request() noexcept(false);

        [[nodiscard]] consteval std::size_t capacity() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool isFull() const noexcept;

        EntityBody<CAPACITY>* begin() noexcept;

        EntityBody<CAPACITY>* end() noexcept;

    private:
        friend class EntityDeleter<CAPACITY>;

        std::array<EntityBody<CAPACITY>, CAPACITY> pool_;
        EntityBody<CAPACITY>* const poolStart_;
        std::array<std::size_t, CAPACITY> stack_;
        std::size_t stackTop_;
        std::size_t size_;
        std::mutex mutex_;
        const EntityDeleter<CAPACITY> entDeleter_;

        void release(EntityBody<CAPACITY>* entBody) noexcept;
    };


    template <std::size_t CAPACITY>
    EntitiesPool<CAPACITY>::EntitiesPool() noexcept
        : pool_{}
        , poolStart_{ pool_.data() }
        , stack_{}
        , stackTop_{ 0U }
        , size_{ 0U }
        , mutex_{}
        , entDeleter_{ *this }
    {
        for (std::size_t i{ 0U }; i != CAPACITY; ++i)
        {
            stack_[i] = i;
        }
    }

    template <std::size_t CAPACITY>
    PooledEntityBody<CAPACITY> EntitiesPool<CAPACITY>::request() noexcept(false)
    {
        std::lock_guard lock{ mutex_ };

        if (stackTop_ == CAPACITY) [[unlikely]]
        {
            throw entities_max_capacity_exception{};
        }

        ++stackTop_;

        ++size_;
        
        return { new (&pool_[stack_[stackTop_ - 1U]]) EntityBody<CAPACITY>{}, entDeleter_ };
    }

    template <std::size_t CAPACITY>
    void EntitiesPool<CAPACITY>::release(EntityBody<CAPACITY>* entBody) noexcept
    {
        std::lock_guard lock{ mutex_ };

        const std::size_t freedObjIdx{ static_cast<std::size_t>(entBody - poolStart_) };

        for (PooledVariant<CAPACITY>& component : entBody->components_)
        {
            component = std::move(std::monostate{});
        }

        --stackTop_;
        stack_[stackTop_] = freedObjIdx;

        --size_;
    }

    template <std::size_t CAPACITY>
    consteval std::size_t EntitiesPool<CAPACITY>::capacity() const noexcept
    {
        return CAPACITY;
    }

    template <std::size_t CAPACITY>
    std::size_t EntitiesPool<CAPACITY>::size() const noexcept
    {
        return size_;
    }

    template <std::size_t CAPACITY>
    bool EntitiesPool<CAPACITY>::isFull() const noexcept
    {
        return size_ == CAPACITY;
    }

    template <std::size_t CAPACITY>
    EntityBody<CAPACITY>* EntitiesPool<CAPACITY>::begin() noexcept
    {
        return poolStart_;
    }

    template <std::size_t CAPACITY>
    EntityBody<CAPACITY>* EntitiesPool<CAPACITY>::end() noexcept
    {
        return poolStart_ + CAPACITY;
    }
}


#endif // !ENTITIES_OBJECT_POOL
