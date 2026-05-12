#include <cstring>

#include <enet/enet.h>

#include <wingz/net/connection.h>

namespace wingz::net
{

struct Connection::Impl
{
    _ENetPeer* peer = nullptr;
    PeerId id = kInvalidPeerId;
};

Connection::Connection(_ENetPeer* peer)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->peer = peer;
    m_impl->id = static_cast<PeerId>(reinterpret_cast<uintptr_t>(peer));
}

Connection::~Connection()
{
    if (m_impl->peer)
        disconnect();
}

void Connection::sendReliable(const Message& msg)
{
    if (!m_impl->peer)
        return;

    ENetPacket* packet = enet_packet_create(
        &msg.header, sizeof(MessageHeader) + msg.data.size(),
        ENET_PACKET_FLAG_RELIABLE
    );
    if (!msg.data.empty())
        std::memcpy(packet->data + sizeof(MessageHeader), msg.data.data(), msg.data.size());

    // Копируем заголовок
    std::memcpy(packet->data, &msg.header, sizeof(MessageHeader));

    enet_peer_send(m_impl->peer, 0, packet);
}

void Connection::sendUnreliable(const Message& msg)
{
    if (!m_impl->peer)
        return;

    // Для ненадёжных сообщений используем флаг UNSEQUENCED
    ENetPacket* packet = enet_packet_create(
        nullptr, sizeof(MessageHeader) + msg.data.size(),
        ENET_PACKET_FLAG_UNSEQUENCED
    );
    std::memcpy(packet->data, &msg.header, sizeof(MessageHeader));
    if (!msg.data.empty())
    {
        std::memcpy(
            packet->data + sizeof(MessageHeader),
            msg.data.data(),
            msg.data.size()
        );
    }

    enet_peer_send(m_impl->peer, 1, packet); // канал 1 — ненадёжный
}

void Connection::disconnect()
{
    if (m_impl->peer)
    {
        enet_peer_disconnect(m_impl->peer, 0);
        m_impl->peer = nullptr;
    }
}

PeerId Connection::id() const
{
    return m_impl->id;
}

bool Connection::isConnected() const
{
    return m_impl->peer && m_impl->peer->state == ENET_PEER_STATE_CONNECTED;
}

} // namespace wingz::net
