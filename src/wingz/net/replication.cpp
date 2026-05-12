#include <cstring>
#include <vector>

#include <spdlog/spdlog.h>

#include <wingz/net/host.h>
#include <wingz/net/replication.h>

#include <wingz/ecs/components.h>
#include <wingz/physics/collider.h>

namespace wingz::net
{

struct ReplicationSystem::Impl
{
    NetEntityId nextNetId = 1;

    struct SerializedEntity
    {
        NetEntityId netId;
        uint8_t playerId;
        float x;
        float y;
        float rot;
        float vx;
        float vy;
        float drot;
        float r;
        float g;
        float b;
        float a;
    };
};

ReplicationSystem::ReplicationSystem()
    : m_impl(std::make_unique<Impl>())
{
}

ReplicationSystem::~ReplicationSystem() = default;

void ReplicationSystem::registerEntity(
    entt::registry& registry,
    entt::entity entity
)
{
    auto& net = registry.get_or_emplace<Networked>(entity);
    if (net.netId == kInvalidNetEntityId)
    {
        net.netId = m_impl->nextNetId++;
    }
}

void ReplicationSystem::unregisterEntity(
    entt::registry& registry,
    entt::entity entity
)
{
    // nothing
}

void ReplicationSystem::serverUpdate(
    entt::registry& registry,
    Host& host,
    TickNumber tick
)
{
    std::vector<Impl::SerializedEntity> entities;

    auto view = registry.view<
        const Networked,
        const ecs::Transform,
        const ecs::Velocity,
        const ecs::Sprite,
        const ecs::Player>();

    for (auto entity : view)
    {
        const auto& net = view.get<const Networked>(entity);
        const auto& transform = view.get<const ecs::Transform>(entity);
        const auto& velocity = view.get<const ecs::Velocity>(entity);
        const auto& sprite = view.get<const ecs::Sprite>(entity);
        const auto& player = view.get<const ecs::Player>(entity);

        Impl::SerializedEntity se;
        se.netId = net.netId;
        se.playerId = player.id;
        se.x = transform.x;
        se.y = transform.y;
        se.rot = transform.rot;
        se.vx = velocity.dx;
        se.vy = velocity.dy;
        se.drot = velocity.drot;
        se.r = sprite.r;
        se.g = sprite.g;
        se.b = sprite.b;
        se.a = sprite.a;

        entities.push_back(se);
    }

    const size_t dataSize = sizeof(uint32_t)
        + entities.size() * sizeof(Impl::SerializedEntity);
    Message msg;
    msg.header.type = MessageType::WorldState;
    msg.header.tick = tick;
    msg.data.resize(dataSize);

    const uint32_t count = static_cast<uint32_t>(entities.size());
    std::memcpy(msg.data.data(), &count, sizeof(uint32_t));
    if (!entities.empty())
    {
        std::memcpy(
            msg.data.data() + sizeof(uint32_t),
            entities.data(),
            entities.size() * sizeof(Impl::SerializedEntity)
        );
    }

    host.broadcast(msg, false);
}

void ReplicationSystem::clientReceive(
    entt::registry& registry,
    const Message& msg
)
{
    if (msg.header.type != MessageType::WorldState)
        return;

    if (msg.data.size() < sizeof(uint32_t))
        return;

    uint32_t count;
    std::memcpy(&count, msg.data.data(), sizeof(uint32_t));

    if (msg.data.size() < sizeof(uint32_t) + count * sizeof(Impl::SerializedEntity))
        return;

    const auto* entities = reinterpret_cast<const Impl::SerializedEntity*>(
        msg.data.data() + sizeof(uint32_t)
    );

    auto view = registry.view<Networked, ecs::Transform, ecs::Velocity>();

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& state = entities[i];

        entt::entity found = entt::null;
        for (auto entity : view)
        {
            const auto& net = view.get<const Networked>(entity);
            if (net.netId == state.netId)
            {
                found = entity;
                break;
            }
        }

        if (found != entt::null)
        {
            auto& transform = registry.get<ecs::Transform>(found);
            auto& velocity = registry.get<ecs::Velocity>(found);
            auto& sprite = registry.get<ecs::Sprite>(found);

            transform.x = state.x;
            transform.y = state.y;
            transform.rot = state.rot;
            velocity.dx = state.vx;
            velocity.dy = state.vy;
            velocity.drot = state.drot;
            sprite.r = state.r;
            sprite.g = state.g;
            sprite.b = state.b;
            sprite.a = state.a;
        }
        else
        {
            auto entity = registry.create();
            registry.emplace<Networked>(entity, state.netId, 0);
            registry.emplace<ecs::Transform>(entity, state.x, state.y, state.rot);
            registry.emplace<ecs::Velocity>(entity, state.vx, state.vy, state.drot);
            registry.emplace<ecs::Sprite>(
                entity, 0u, 0.0f, 0.0f, 1.0f, 1.0f, 48.0f, 48.0f,
                state.r, state.g, state.b, state.a
            );
            registry.emplace<ecs::Player>(entity, state.playerId);
            registry.emplace<ecs::InputIntent>(entity, 0.0f, 0.0f, false);

            const std::string tag = (state.playerId == 0)
                ? "ServerPlayer"
                : "ClientPlayer";
            registry.emplace<ecs::Tag>(entity, tag);
        }
    }
}

} // namespace wingz::net
