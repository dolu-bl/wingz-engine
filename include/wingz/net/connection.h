#pragma once

#include <memory>

#include <wingz/net/types.h>

// Forward-декларации для ENet
struct _ENetHost;
struct _ENetPeer;
struct _ENetPacket;

namespace wingz::net
{

/// Обёртка над ENet-пиром.
class Connection
{
public:
    Connection(_ENetPeer* peer);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// Отправить надёжное сообщение.
    void sendReliable(const Message& msg);

    /// Отправить ненадёжное сообщение (для состояния мира).
    void sendUnreliable(const Message& msg);

    /// Отключить пира.
    void disconnect();

    PeerId id() const;
    bool isConnected() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::net
