#pragma once

#include <memory>

#include <entt/entt.hpp>

#include <wingz/net/types.h>

namespace wingz::core
{
class AssetManager;
}

namespace wingz::net
{

class Host;

/// Компонент, помечающий сущность как реплицируемую.
struct Networked
{
    NetEntityId netId = kInvalidNetEntityId;
    PeerId ownerId = kInvalidPeerId;
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

    /// Установить AssetManager для клиента.
    void setAssetManager(core::AssetManager* assets) { m_assets = assets; }

    /// Сервер: собрать состояние мира и разослать клиентам.
    void serverUpdate(entt::registry& registry, Host& host, TickNumber tick);

    /// Клиент: применить полученное состояние мира.
    void clientReceive(entt::registry& registry, const Message& msg);

    /// Клиент: десериализовать WorldState в список состояний.
    std::vector<SerializedEntityState> deserializeWorldState(const Message& msg) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    core::AssetManager* m_assets = nullptr;
};

} // namespace wingz::net
