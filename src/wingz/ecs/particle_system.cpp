#include <algorithm>
#include <cmath>

#include <entt/entt.hpp>

#include <wingz/ecs/components.h>
#include <wingz/ecs/particle.h>
#include <wingz/ecs/particle_system.h>

namespace wingz::ecs
{

struct ParticleSystem::Impl
{
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist01; // 0..1
    std::uniform_real_distribution<float> distAngle; // -PI..PI

    Impl()
        : rng(std::random_device {}())
        , dist01(0.0f, 1.0f)
        , distAngle(-3.14159265f, 3.14159265f)
    {
    }

    float randomFloat(float min, float max)
    {
        return min + dist01(rng) * (max - min);
    }
};

ParticleSystem::ParticleSystem()
    : m_impl(std::make_unique<Impl>())
{
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::update(entt::registry& registry, float dt)
{
    // ────────────────────────────────────────
    // 1. Обновляем эмиттеры — создаём новые частицы
    // ────────────────────────────────────────
    auto emitterView = registry.view<Transform, ParticleEmitter>();
    for (auto entity : emitterView)
    {
        auto& transform = emitterView.get<Transform>(entity);
        auto& emitter = emitterView.get<ParticleEmitter>(entity);

        if (!emitter.active)
            continue;

        // Подсчитываем текущее количество живых частиц от этого эмиттера
        // (частицы не имеют прямой ссылки на эмиттер, поэтому используем подсчёт
        // всех частиц с типом эмиттера — упрощение для песочницы)
        // TODO: добавить поле emitterEntity в Particle

        // Спавн по таймеру
        emitter.spawnTimer -= dt;
        while (emitter.spawnTimer <= 0.0f && emitter.active)
        {
            emitter.spawnTimer += emitter.spawnInterval;

            if (emitter.oneShot)
            {
                // Однократный выброс
                if (emitter.spawnedCount < emitter.totalBurstCount)
                {
                    float angle = emitter.baseAngle
                        + m_impl->distAngle(m_impl->rng) * emitter.spreadAngle * 0.5f;
                    float speed = emitter.baseSpeed
                        + m_impl->randomFloat(-emitter.speedVariance, emitter.speedVariance);
                    float vx = std::cos(angle) * speed;
                    float vy = std::sin(angle) * speed;

                    Particle p;
                    p.lifetime = emitter.baseLifetime
                        + m_impl->randomFloat(-emitter.lifetimeVariance, emitter.lifetimeVariance);
                    p.lifetime = std::max(0.05f, p.lifetime);
                    p.elapsed = 0.0f;
                    p.startR = emitter.startR;
                    p.startG = emitter.startG;
                    p.startB = emitter.startB;
                    p.startA = emitter.startA;
                    p.endR = emitter.endR;
                    p.endG = emitter.endG;
                    p.endB = emitter.endB;
                    p.endA = emitter.endA;
                    p.startWidth = emitter.startWidth;
                    p.startHeight = emitter.startHeight;
                    p.endWidth = emitter.endWidth;
                    p.endHeight = emitter.endHeight;
                    p.fadeOut = emitter.fadeOut;
                    p.flicker = emitter.flicker;
                    p.type = emitter.particleType;

                    Sprite sprite;
                    sprite.textureId = 0;
                    sprite.width = emitter.startWidth;
                    sprite.height = emitter.startHeight;
                    sprite.r = emitter.startR;
                    sprite.g = emitter.startG;
                    sprite.b = emitter.startB;
                    sprite.a = emitter.startA;

                    spawnParticle(registry, transform.x, transform.y, vx, vy, p, sprite);
                    ++emitter.spawnedCount;
                }
                else
                {
                    emitter.active = false;
                }
            }
            else
            {
                // Постоянный эмиттер — создаём burstCount частиц
                for (uint32_t i = 0; i < emitter.burstCount; ++i)
                {
                    float angle = emitter.baseAngle
                        + m_impl->distAngle(m_impl->rng) * emitter.spreadAngle * 0.5f;
                    float speed = emitter.baseSpeed
                        + m_impl->randomFloat(-emitter.speedVariance, emitter.speedVariance);
                    float vx = std::cos(angle) * speed;
                    float vy = std::sin(angle) * speed;

                    Particle p;
                    p.lifetime = emitter.baseLifetime
                        + m_impl->randomFloat(-emitter.lifetimeVariance, emitter.lifetimeVariance);
                    p.lifetime = std::max(0.05f, p.lifetime);
                    p.elapsed = 0.0f;
                    p.startR = emitter.startR;
                    p.startG = emitter.startG;
                    p.startB = emitter.startB;
                    p.startA = emitter.startA;
                    p.endR = emitter.endR;
                    p.endG = emitter.endG;
                    p.endB = emitter.endB;
                    p.endA = emitter.endA;
                    p.startWidth = emitter.startWidth;
                    p.startHeight = emitter.startHeight;
                    p.endWidth = emitter.endWidth;
                    p.endHeight = emitter.endHeight;
                    p.fadeOut = emitter.fadeOut;
                    p.flicker = emitter.flicker;
                    p.type = emitter.particleType;

                    Sprite sprite;
                    sprite.textureId = 0;
                    sprite.width = emitter.startWidth;
                    sprite.height = emitter.startHeight;
                    sprite.r = emitter.startR;
                    sprite.g = emitter.startG;
                    sprite.b = emitter.startB;
                    sprite.a = emitter.startA;

                    spawnParticle(registry, transform.x, transform.y, vx, vy, p, sprite);
                }
            }
        }
    }

    // ────────────────────────────────────────
    // 2. Обновляем существующие частицы
    // ────────────────────────────────────────
    auto particleView = registry.view<Particle, Transform, Velocity, Sprite>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : particleView)
    {
        auto& particle = particleView.get<Particle>(entity);
        auto& transform = particleView.get<Transform>(entity);
        auto& velocity = particleView.get<Velocity>(entity);
        auto& sprite = particleView.get<Sprite>(entity);

        // Обновляем время жизни
        particle.elapsed += dt;

        if (particle.elapsed >= particle.lifetime)
        {
            toDestroy.push_back(entity);
            continue;
        }

        // Двигаем частицу
        transform.x += velocity.dx * dt;
        transform.y += velocity.dy * dt;

        // Затухание скорости (трение)
        velocity.dx *= 0.98f;
        velocity.dy *= 0.98f;

        // Интерполяция размера
        float t = particle.elapsed / particle.lifetime;
        sprite.width = particle.startWidth + (particle.endWidth - particle.startWidth) * t;
        sprite.height = particle.startHeight + (particle.endHeight - particle.startHeight) * t;

        // Интерполяция цвета
        sprite.r = particle.startR + (particle.endR - particle.startR) * t;
        sprite.g = particle.startG + (particle.endG - particle.startG) * t;
        sprite.b = particle.startB + (particle.endB - particle.startB) * t;

        // Прозрачность
        if (particle.fadeOut)
        {
            sprite.a = particle.startA + (particle.endA - particle.startA) * t;
        }

        // Мерцание
        if (particle.flicker)
        {
            float flickerValue = 0.5f + 0.5f * std::sin(particle.elapsed * 20.0f);
            sprite.a *= flickerValue;
        }

        // Поворот от скорости
        transform.rot = std::atan2(velocity.dy, velocity.dx);
    }

    // Уничтожаем мёртвые частицы
    for (auto entity : toDestroy)
    {
        registry.destroy(entity);
    }
}

entt::entity ParticleSystem::spawnParticle(
    entt::registry& registry,
    float x, float y,
    float vx, float vy,
    const Particle& particle,
    const Sprite& sprite
)
{
    auto entity = registry.create();
    registry.emplace<Transform>(entity, x, y, 0.0f);
    registry.emplace<Velocity>(entity, vx, vy, 0.0f);
    registry.emplace<Sprite>(entity, sprite);
    registry.emplace<Particle>(entity, particle);
    registry.emplace<Tag>(entity, "Particle");
    return entity;
}

void ParticleSystem::emitBurst(
    entt::registry& registry,
    float x, float y,
    const ParticleEmitter& emitter,
    uint32_t count
)
{
    Impl impl;

    for (uint32_t i = 0; i < count; ++i)
    {
        float angle = emitter.baseAngle
            + impl.distAngle(impl.rng) * emitter.spreadAngle * 0.5f;
        float speed = emitter.baseSpeed
            + impl.randomFloat(-emitter.speedVariance, emitter.speedVariance);
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;

        Particle p;
        p.lifetime = emitter.baseLifetime
            + impl.randomFloat(-emitter.lifetimeVariance, emitter.lifetimeVariance);
        p.lifetime = std::max(0.05f, p.lifetime);
        p.elapsed = 0.0f;
        p.startR = emitter.startR;
        p.startG = emitter.startG;
        p.startB = emitter.startB;
        p.startA = emitter.startA;
        p.endR = emitter.endR;
        p.endG = emitter.endG;
        p.endB = emitter.endB;
        p.endA = emitter.endA;
        p.startWidth = emitter.startWidth;
        p.startHeight = emitter.startHeight;
        p.endWidth = emitter.endWidth;
        p.endHeight = emitter.endHeight;
        p.fadeOut = emitter.fadeOut;
        p.flicker = emitter.flicker;
        p.type = emitter.particleType;

        Sprite sprite;
        sprite.textureId = 0;
        sprite.width = emitter.startWidth;
        sprite.height = emitter.startHeight;
        sprite.r = emitter.startR;
        sprite.g = emitter.startG;
        sprite.b = emitter.startB;
        sprite.a = emitter.startA;

        spawnParticle(registry, x, y, vx, vy, p, sprite);
    }
}

} // namespace wingz::ecs
