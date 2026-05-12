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
    // Собираем состояния сущностей
    std::vector<SerializedEntityState> states;

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

        SerializedEntityState st;
        st.netId = net.netId;
        st.playerId = static_cast<uint8_t>(player.id);
        st.x = transform.x;
        st.y = transform.y;
        st.rot = transform.rot;
        st.vx = velocity.dx;
        st.vy = velocity.dy;
        st.drot = velocity.drot;
        st.r = sprite.r;
        st.g = sprite.g;
        st.b = sprite.b;
        st.a = sprite.a;

        states.push_back(st);
    }

    // Сериализуем через Serializer
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

    Serializer ser(msg.data);

    if (ser.remainingBytes() < sizeof(uint32_t))
    {
        spdlog::warn("ReplicationSystem::clientReceive: сообщение слишком короткое");
        return;
    }

    uint32_t count = ser.readU32();

    // Проверяем, что данных хватает на все сущности
    const size_t expectedBytes = count * (sizeof(uint32_t) // netId
                                          + sizeof(uint8_t) // playerId
                                          + sizeof(float) * 10 // 10 полей float
                                 );

    if (ser.remainingBytes() < expectedBytes)
    {
        spdlog::warn(
            "ReplicationSystem::clientReceive: ожидалось {} байт, но осталось {}",
            expectedBytes,
            ser.remainingBytes()
        );
        return;
    }

    // Читаем состояния
    std::vector<SerializedEntityState> states;
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

        states.push_back(st);
    }

    // Применяем состояния к локальным сущностям
    auto view = registry.view<Networked, ecs::Transform, ecs::Velocity, ecs::Sprite>();

    for (const auto& st : states)
    {
        entt::entity found = entt::null;

        // Ищем существующую сущность по netId
        for (auto entity : view)
        {
            const auto& net = view.get<const Networked>(entity);
            if (net.netId == st.netId)
            {
                found = entity;
                break;
            }
        }

        if (found != entt::null)
        {
            // Обновляем существующую
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

            // Обновляем Player, если компонент есть
            auto* player = registry.try_get<ecs::Player>(found);
            if (player)
                player->id = st.playerId;
        }
        else
        {
            // Создаём новую сущность
            auto entity = registry.create();
            registry.emplace<Networked>(entity, st.netId, 0);
            registry.emplace<ecs::Transform>(entity, st.x, st.y, st.rot);
            registry.emplace<ecs::Velocity>(entity, st.vx, st.vy, st.drot);
            registry.emplace<ecs::Sprite>(
                entity, 0u, 0.0f, 0.0f, 1.0f, 1.0f, 48.0f, 48.0f,
                st.r, st.g, st.b, st.a
            );
            registry.emplace<ecs::Player>(entity, static_cast<uint32_t>(st.playerId));
            registry.emplace<ecs::InputIntent>(entity, 0.0f, 0.0f, false);

            const std::string tag = (st.playerId == 0)
                ? "ServerPlayer"
                : "ClientPlayer";
            registry.emplace<ecs::Tag>(entity, tag);
        }
    }
}

} // namespace wingz::net
