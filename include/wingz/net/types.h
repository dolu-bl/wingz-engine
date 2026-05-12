#pragma once

#include <cstdint>
#include <vector>

namespace wingz::net
{

/// Идентификатор пира.
using PeerId = uint32_t;
constexpr PeerId kInvalidPeerId = 0xFFFFFFFF;

/// Идентификатор сетевой сущности.
using NetEntityId = uint32_t;
constexpr NetEntityId kInvalidNetEntityId = 0;

/// Тик сервера (монотонно возрастающий).
using TickNumber = uint32_t;

/// Типы сетевых сообщений.
enum class MessageType : uint8_t
{
    // Соединение
    ConnectRequest,
    ConnectAccept,
    ConnectReject,
    Disconnect,

    // Игровые данные
    InputState,
    WorldState,
    EntitySpawn,
    EntityDestroy,

    // Служебные
    Ping,
    Pong
};

/// Заголовок сообщения.
struct MessageHeader
{
    MessageType type;
    TickNumber tick;
    PeerId senderId;
};

/// Сообщение с данными.
struct Message
{
    MessageHeader header;
    std::vector<uint8_t> data;
};

/// Пакет ввода от клиента.
struct InputPacket
{
    TickNumber tick;
    float moveX;
    float moveY;
    bool fire;
};

} // namespace wingz::net
