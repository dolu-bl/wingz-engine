#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>

#include <wingz/net/types.h>

namespace wingz::net
{

/// Интерполированное состояние одной сущности на клиенте.
struct InterpolatedState
{
    float x = 0.0f;
    float y = 0.0f;
    float rot = 0.0f;
    bool active = false;
};

/// Буферизирует снапшоты с сервера и выдаёт интерполированные положения.
/// Клиент намеренно отстаёт на kBufferTicks тиков для плавности.
class ClientInterpolation
{
public:
    ClientInterpolation();
    ~ClientInterpolation();

    ClientInterpolation(const ClientInterpolation&) = delete;
    ClientInterpolation& operator=(const ClientInterpolation&) = delete;

    /// Вызывается при получении WorldState от сервера.
    /// Сохраняет снапшот для будущей интерполяции.
    void pushSnapshot(
        TickNumber tick,
        const std::vector<SerializedEntityState>& states
    );

    /// Вызывается каждый кадр перед рендером.
    /// Вычисляет интерполированное положение для всех известных сущностей.
    /// @param currentTick Текущий тик клиента (отстаёт от серверного).
    /// @param dt Время с последнего кадра в секундах.
    /// @param tickDuration Длительность одного тика в секундах (например, 1.0f / 30.0f).
    void update(TickNumber currentTick, float dt, float tickDuration);

    /// Получить интерполированное состояние для сущности по её netId.
    /// Возвращает nullptr, если сущность не найдена.
    const InterpolatedState* getState(NetEntityId netId) const;

    /// Принудительно очистить буфер (при дисконнекте).
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::net
