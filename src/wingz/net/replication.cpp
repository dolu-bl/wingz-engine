#include <vector>

#include <spdlog/spdlog.h>

#include <wingz/ecs/components.h>
#include <wingz/net/host.h>
#include <wingz/net/replication.h>
#include <wingz/net/serializer.h>

namespace wingz::net
{

struct ReplicationSystem::Impl
{
    NetEntityId nextNetId = 1;
    std::vector<SerializedEntityState> stateBuffer;
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
    // Пока ничего
}

void ReplicationSystem::serverUpdate(
    entt::registry& registry,
    Host& host,
    TickNumber tick
)
{
    auto& states = m_impl->stateBuffer;
    states.clear();

    // Собираем ВСЕ сущности с Networked и Sprite
    // Игроки имеют Player+Velocity, стены — нет
    auto view = registry.view<const Networked, const ecs::Transform, const ecs::Sprite>();

    for (auto entity : view)
    {
        const auto& net = view.get<const Networked>(entity);
        const auto& transform = view.get<const ecs::Transform>(entity);
        const auto& sprite = view.get<const ecs::Sprite>(entity);

        SerializedEntityState st;
        st.netId = net.netId;

        // Игрок или стена?
        const auto* player = registry.try_get<ecs::Player>(entity);
        if (player)
        {
            st.playerId = static_cast<uint8_t>(player->id);
            const auto& velocity = registry.get<ecs::Velocity>(entity);
            st.vx = velocity.dx;
            st.vy = velocity.dy;
            st.drot = velocity.drot;
        }
        else
        {
            st.playerId = 0xFF;
            st.vx = 0.0f;
            st.vy = 0.0f;
            st.drot = 0.0f;
        }

        st.x = transform.x;
        st.y = transform.y;
        st.rot = transform.rot;
        st.r = sprite.r;
        st.g = sprite.g;
        st.b = sprite.b;
        st.a = sprite.a;
        st.width = sprite.width;
        st.height = sprite.height;

        // Здоровье
        const auto* health = registry.try_get<ecs::Health>(entity);
        if (health)
        {
            st.health = health->current;
            st.maxHealth = health->max;
        }
        else
        {
            st.health = -1.0f;
            st.maxHealth = -1.0f;
        }

        states.push_back(st);
    }

    // Сериализация
    Message msg;
    msg.header.type = MessageType::WorldState;
    msg.header.tick = tick;

    Serializer ser(msg.data);
    ser.writeU32(static_cast<uint32_t>(states.size()));

    for (const auto& st : states)
    {
        ser.writeU32(st.netId);
        ser.writeU8(st.playerId);
        ser.writeF32(st.x);
        ser.writeF32(st.y);
        ser.writeF32(st.rot);
        ser.writeF32(st.vx);
        ser.writeF32(st.vy);
        ser.writeF32(st.drot);
        ser.writeF32(st.r);
        ser.writeF32(st.g);
        ser.writeF32(st.b);
        ser.writeF32(st.a);
        ser.writeF32(st.health);
        ser.writeF32(st.maxHealth);
        ser.writeF32(st.width);
        ser.writeF32(st.height);
    }

    host.broadcast(msg, false);
}

void ReplicationSystem::clientReceive(
    entt::registry& registry,
    const Message& msg
)
{
    auto states = deserializeWorldState(msg);
    if (states.empty())
        return;

    // Собираем ID, которые есть в этом снапшоте
    std::unordered_set<NetEntityId> receivedIds;
    for (const auto& st : states)
        receivedIds.insert(st.netId);

    // Применяем состояния
    auto view = registry.view<Networked, ecs::Transform, ecs::Velocity, ecs::Sprite>();

    for (const auto& st : states)
    {
        // Ищем существующую сущность
        entt::entity found = entt::null;
        for (auto entity : view)
        {
            if (view.get<Networked>(entity).netId == st.netId)
            {
                found = entity;
                break;
            }
        }

        if (found != entt::null)
        {
            // Обновляем
            auto& transform = view.get<ecs::Transform>(found);
            auto& velocity = view.get<ecs::Velocity>(found);
            auto& sprite = view.get<ecs::Sprite>(found);

            transform.x = st.x;
            transform.y = st.y;
            transform.rot = st.rot;
            velocity.dx = st.vx;
            velocity.dy = st.vy;
            velocity.drot = st.drot;
            sprite.r = st.r;
            sprite.g = st.g;
            sprite.b = st.b;
            sprite.a = st.a;
            sprite.width = st.width;
            sprite.height = st.height;

            // Здоровье
            if (st.health >= 0.0f)
            {
                auto* health = registry.try_get<ecs::Health>(found);
                if (health)
                {
                    health->current = st.health;
                    health->max = st.maxHealth;
                }
                else
                {
                    registry.emplace<ecs::Health>(found, st.health, st.maxHealth);
                }
            }
        }
        else
        {
            // Создаём новую
            auto entity = registry.create();
            registry.emplace<Networked>(entity, st.netId, 0);
            registry.emplace<ecs::Transform>(entity, st.x, st.y, st.rot);
            registry.emplace<ecs::Velocity>(entity, st.vx, st.vy, st.drot);
            registry.emplace<ecs::Sprite>(
                entity,
                0u, 0.0f, 0.0f, 1.0f, 1.0f,
                st.width, st.height,
                st.r, st.g, st.b, st.a
            );

            if (st.playerId != 0xFF)
            {
                registry.emplace<ecs::Player>(entity, static_cast<uint32_t>(st.playerId));
                registry.emplace<ecs::InputIntent>(entity, 0.0f, 0.0f, false);
                registry.emplace<ecs::Tag>(entity, (st.playerId == 0) ? "ServerPlayer" : "ClientPlayer");
            }
            else
            {
                registry.emplace<ecs::Tag>(entity, "Wall");
            }

            if (st.health >= 0.0f)
                registry.emplace<ecs::Health>(entity, st.health, st.maxHealth);
        }
    }

    // Удаляем сущности, которых нет в снапшоте (уничтожены на сервере)
    std::vector<entt::entity> toRemove;
    for (auto entity : view)
    {
        if (receivedIds.find(view.get<Networked>(entity).netId) == receivedIds.end())
            toRemove.push_back(entity);
    }

    for (auto entity : toRemove)
        registry.destroy(entity);
}

std::vector<SerializedEntityState> ReplicationSystem::deserializeWorldState(
    const Message& msg
) const
{
    std::vector<SerializedEntityState> states;

    if (msg.header.type != MessageType::WorldState)
        return states;

    Serializer ser(msg.data);

    if (ser.remainingBytes() < sizeof(uint32_t))
        return states;

    uint32_t count = ser.readU32();

    // 14 полей float: x, y, rot, vx, vy, drot, r, g, b, a, health, maxHealth, width, height
    const size_t perEntityBytes = sizeof(uint32_t)
        + sizeof(uint8_t)
        + sizeof(float)
            * 14;

    if (ser.remainingBytes() < count * perEntityBytes)
    {
        spdlog::warn("ReplicationSystem: недостаточно данных в WorldState");
        return states;
    }

    states.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        SerializedEntityState st;
        st.netId = ser.readU32();
        st.playerId = ser.readU8();
        st.x = ser.readF32();
        st.y = ser.readF32();
        st.rot = ser.readF32();
        st.vx = ser.readF32();
        st.vy = ser.readF32();
        st.drot = ser.readF32();
        st.r = ser.readF32();
        st.g = ser.readF32();
        st.b = ser.readF32();
        st.a = ser.readF32();
        st.health = ser.readF32();
        st.maxHealth = ser.readF32();
        st.width = ser.readF32();
        st.height = ser.readF32();

        states.push_back(st);
    }

    return states;
}

} // namespace wingz::net
