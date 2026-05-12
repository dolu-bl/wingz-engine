#pragma once

#include <memory>

#include <entt/entt.hpp>

#include <wingz/net/types.h>

namespace wingz::net
{

class Host;

/// Компонент, помечающий сущность как реплицируемую.
struct Networked
{
    NetEntityId netId = kInvalidNetEntityId;
    PeerId ownerId = kInvalidPeerId; // кто владеет (обычно сервер)
};

/// Система репликации: синхронизирует состояние между сервером и клиентами.
class ReplicationSystem
{
public:
    ReplicationSystem();
    ~ReplicationSystem();

    ReplicationSystem(const ReplicationSystem&) = delete;
    ReplicationSystem& operator=(const ReplicationSystem&) = delete;

    /// Назначить NetEntityId новой сущности (только на сервере).
    void registerEntity(entt::registry& registry, entt::entity entity);

    /// Удалить сущность из репликации.
    void unregisterEntity(entt::registry& registry, entt::entity entity);

    /// Сервер: собрать состояние мира и разослать клиентам.
    void serverUpdate(entt::registry& registry, Host& host, TickNumber tick);

    /// Клиент: применить полученное состояние мира.
    void clientReceive(entt::registry& registry, const Message& msg);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::net
