
#include <cstring>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include <enet/enet.h>

#include <wingz/net/host.h>

namespace wingz::net
{

struct Host::Impl
{
    _ENetHost* host = nullptr;
    bool isServer = false;
    PeerId localId = kInvalidPeerId;
    std::unordered_map<PeerId, _ENetPeer*> peers;
    static bool enetInitialized;
};

bool Host::Impl::enetInitialized = false;

Host::Host()
    : m_impl(std::make_unique<Impl>())
{
    if (!Impl::enetInitialized)
    {
        if (enet_initialize() != 0)
            throw std::runtime_error("Не удалось инициализировать ENet");
        Impl::enetInitialized = true;
    }
}

Host::~Host()
{
    if (m_impl->host)
    {
        enet_host_destroy(m_impl->host);
        m_impl->host = nullptr;
    }
}

std::unique_ptr<Host> Host::createServer(uint16_t port, uint32_t maxClients)
{
    auto host = std::unique_ptr<Host>(new Host());

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    host->m_impl->host = enet_host_create(&address, maxClients, 2, 0, 0);
    if (!host->m_impl->host)
        throw std::runtime_error("Не удалось создать ENet-сервер на порту " + std::to_string(port));

    host->m_impl->isServer = true;
    host->m_impl->localId = 0; // сервер всегда имеет ID 0

    spdlog::info("Сервер запущен на порту {}", port);
    return host;
}

std::unique_ptr<Host> Host::createClient()
{
    auto host = std::unique_ptr<Host>(new Host());

    host->m_impl->host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host->m_impl->host)
        throw std::runtime_error("Не удалось создать ENet-клиент");

    host->m_impl->isServer = false;

    spdlog::info("Клиент создан");
    return host;
}

bool Host::connect(const std::string& address, uint16_t port)
{
    if (m_impl->isServer)
    {
        spdlog::error("Сервер не может подключаться к другому серверу");
        return false;
    }

    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;

    _ENetPeer* peer = enet_host_connect(m_impl->host, &addr, 2, 0);
    if (!peer)
    {
        spdlog::error("Не удалось подключиться к {}:{}", address, port);
        return false;
    }

    m_impl->localId = static_cast<PeerId>(reinterpret_cast<uintptr_t>(peer));
    spdlog::info("Подключение к {}:{}...", address, port);
    return true;
}

void Host::disconnectClient(PeerId peerId)
{
    if (!m_impl->isServer)
        return;

    auto it = m_impl->peers.find(peerId);
    if (it != m_impl->peers.end())
    {
        enet_peer_disconnect(it->second, 0);
        m_impl->peers.erase(it);
    }
}

void Host::disconnect()
{
    if (!m_impl->host)
        return;

    if (!m_impl->isServer && m_impl->host->peerCount > 0)
    {
        enet_peer_disconnect(&m_impl->host->peers[0], 0);
    }
}

void Host::send(PeerId peerId, const Message& msg, bool reliable)
{
    if (!m_impl->host)
        return;

    _ENetPeer* peer = nullptr;
    if (m_impl->isServer)
    {
        auto it = m_impl->peers.find(peerId);
        if (it != m_impl->peers.end())
            peer = it->second;
        else
        {
            spdlog::warn(
                "Host::send: пир {} не найден (всего пиров: {})",
                peerId,
                m_impl->peers.size()
            );
            return;
        }
    }
    else
    {
        // Клиент: отправляем серверу (peerId должен быть 0, но игнорируем)
        if (m_impl->host->peerCount > 0)
            peer = &m_impl->host->peers[0];
        else
        {
            spdlog::warn("Host::send: нет подключения к серверу");
            return;
        }
    }

    if (!peer)
        return;

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
    uint8_t channel = reliable ? 0 : 1;

    size_t totalSize = sizeof(MessageHeader) + msg.data.size();
    ENetPacket* packet = enet_packet_create(nullptr, totalSize, flags);
    std::memcpy(packet->data, &msg.header, sizeof(MessageHeader));
    if (!msg.data.empty())
    {
        std::memcpy(
            packet->data + sizeof(MessageHeader),
            msg.data.data(),
            msg.data.size()
        );
    }

    int result = enet_peer_send(peer, channel, packet);
    if (result < 0)
        spdlog::error("Host::send: enet_peer_send вернул {}", result);
}

void Host::broadcast(const Message& msg, bool reliable, PeerId excludeId)
{
    if (!m_impl->isServer || !m_impl->host)
        return;

    for (auto& [peerId, peer] : m_impl->peers)
    {
        if (peerId == excludeId)
            continue;

        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
        uint8_t channel = reliable ? 0 : 1;

        size_t totalSize = sizeof(MessageHeader) + msg.data.size();
        ENetPacket* packet = enet_packet_create(nullptr, totalSize, flags);
        std::memcpy(packet->data, &msg.header, sizeof(MessageHeader));
        if (!msg.data.empty())
        {
            std::memcpy(
                packet->data + sizeof(MessageHeader),
                msg.data.data(),
                msg.data.size()
            );
        }

        enet_peer_send(peer, channel, packet);
    }
}

void Host::poll(NetEventCallback callback)
{
    if (!m_impl->host || !callback)
        return;

    ENetEvent event;
    while (enet_host_service(m_impl->host, &event, 0) > 0)
    {
        NetEvent netEvent;

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            netEvent.type = NetEvent::Type::Connect;
            netEvent.peerId = static_cast<PeerId>(reinterpret_cast<uintptr_t>(event.peer));
            if (m_impl->isServer)
            {
                m_impl->peers[netEvent.peerId] = event.peer;
                spdlog::info("Клиент {} подключился", netEvent.peerId);
            }
            else
            {
                m_impl->localId = netEvent.peerId;
                spdlog::info("Подключены к серверу, локальный ID: {}", netEvent.peerId);
            }
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            netEvent.type = NetEvent::Type::Disconnect;
            netEvent.peerId = static_cast<PeerId>(reinterpret_cast<uintptr_t>(event.peer));
            if (m_impl->isServer)
            {
                m_impl->peers.erase(netEvent.peerId);
                spdlog::info("Клиент {} отключился", netEvent.peerId);
            }
            else
            {
                spdlog::info("Отключены от сервера");
            }
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            netEvent.type = NetEvent::Type::Data;
            netEvent.peerId = static_cast<PeerId>(reinterpret_cast<uintptr_t>(event.peer));

            ENetPacket* packet = event.packet;
            if (packet->dataLength >= sizeof(MessageHeader))
            {
                std::memcpy(&netEvent.message.header, packet->data, sizeof(MessageHeader));
                size_t dataSize = packet->dataLength - sizeof(MessageHeader);
                if (dataSize > 0)
                {
                    netEvent.message.data.resize(dataSize);
                    std::memcpy(
                        netEvent.message.data.data(),
                        packet->data + sizeof(MessageHeader),
                        dataSize
                    );
                }
            }
            enet_packet_destroy(packet);
            break;
        }
        default:
            continue;
        }

        callback(netEvent);
    }
}

size_t Host::connectedPeers() const
{
    return m_impl->isServer
        ? m_impl->peers.size()
        : (m_impl->host ? m_impl->host->connectedPeers : 0);
}

PeerId Host::localId() const
{
    return m_impl->localId;
}

bool Host::isRunning() const
{
    return m_impl->host != nullptr;
}

} // namespace wingz::net
