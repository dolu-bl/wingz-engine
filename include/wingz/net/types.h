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

/// Сериализованное состояние одной сущности для репликации.
/// Эта структура НЕ передаётся по сети напрямую — используется Serializer.
struct SerializedEntityState
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

} // namespace wingz::net
