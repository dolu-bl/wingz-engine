#pragma once

#include <functional>
#include <memory>

#include <wingz/net/types.h>

namespace wingz::net
{

class Connection;

/// События сети.
struct NetEvent
{
    enum class Type
    {
        None,
        Connect,
        Disconnect,
        Data
    };

    Type type = Type::None;
    PeerId peerId = kInvalidPeerId;
    Message message;
};

/// Колбэк при сетевом событии.
using NetEventCallback = std::function<void(const NetEvent&)>;

/// Хост (сервер или клиент).
class Host
{
public:
    /// Создать серверный хост.
    static std::unique_ptr<Host> createServer(uint16_t port, uint32_t maxClients = 8);

    /// Создать клиентский хост.
    static std::unique_ptr<Host> createClient();

    Host();
    virtual ~Host();

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    /// Подключиться к серверу (только для клиента).
    bool connect(const std::string& address, uint16_t port);

    /// Отключить клиента по ID (только для сервера).
    void disconnectClient(PeerId peerId);

    /// Отключиться от сервера (только для клиента).
    void disconnect();

    /// Отправить сообщение конкретному пиру.
    void send(PeerId peerId, const Message& msg, bool reliable = false);

    /// Отправить сообщение всем подключённым пирам (только для сервера).
    void broadcast(
        const Message& msg,
        bool reliable = false,
        PeerId excludeId = kInvalidPeerId
    );

    /// Обработать входящие события (вызывать каждый кадр).
    void poll(NetEventCallback callback);

    /// Количество подключённых пиров.
    size_t connectedPeers() const;

    /// Локальный ID пира.
    PeerId localId() const;

    /// Запущена ли сеть.
    bool isRunning() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::net
