#pragma once

#include <random>

#include <entt/entt.hpp>

#include <wingz/ecs/components.h>
#include <wingz/ecs/particle.h>

namespace wingz::ecs
{

/// Система частиц: обновляет время жизни, движение, цвет, размер.
/// Уничтожает сущности с истекшим временем жизни.
/// Создаёт новые частицы из эмиттеров.
class ParticleSystem
{
public:
    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    /// Обновить все частицы и эмиттеры.
    /// @param registry ECS-реестр.
    /// @param dt Время с последнего кадра в секундах.
    void update(entt::registry& registry, float dt);

    /// Создать одну частицу в указанной позиции.
    /// @return Созданная сущность.
    static entt::entity spawnParticle(
        entt::registry& registry,
        float x, float y,
        float vx, float vy,
        const Particle& particle,
        const Sprite& sprite
    );

    /// Создать burst частиц из эмиттера (однократный выброс).
    static void emitBurst(
        entt::registry& registry,
        float x, float y,
        const ParticleEmitter& emitter,
        uint32_t count
    );

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::ecs
