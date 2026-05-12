#include <algorithm>
#include <cmath>

#include <entt/entt.hpp>

#include <wingz/ecs/components.h>
#include <wingz/ecs/particle.h>
#include <wingz/ecs/particle_system.h>

namespace wingz::ecs
{

namespace
{

// Вспомогательная функция: создаёт одну частицу из эмиттера
void spawnFromEmitter(
    entt::registry& registry,
    float x, float y,
    const ParticleEmitter& emitter
)
{
    static std::mt19937 rng(std::random_device {}());
    static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    static std::uniform_real_distribution<float> distAngle(-3.14159265f, 3.14159265f);

    float angle = emitter.baseAngle + distAngle(rng) * emitter.spreadAngle * 0.5f;
    float speed = emitter.baseSpeed
        + (dist01(rng) * 2.0f - 1.0f) * emitter.speedVariance;
    float vx = std::cos(angle) * speed;
    float vy = std::sin(angle) * speed;

    Particle p;
    p.lifetime = emitter.baseLifetime
        + (dist01(rng) * 2.0f - 1.0f) * emitter.lifetimeVariance;
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

    ParticleSystem::spawnParticle(registry, x, y, vx, vy, p, sprite);
}

} // namespace

struct ParticleSystem::Impl
{
    // Пусто — всё делается в статических функциях
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

        emitter.spawnTimer -= dt;
        while (emitter.spawnTimer <= 0.0f && emitter.active)
        {
            emitter.spawnTimer += emitter.spawnInterval;

            if (emitter.oneShot)
            {
                if (emitter.spawnedCount < emitter.totalBurstCount)
                {
                    spawnFromEmitter(registry, transform.x, transform.y, emitter);
                    ++emitter.spawnedCount;
                }
                else
                {
                    emitter.active = false;
                }
            }
            else
            {
                for (uint32_t i = 0; i < emitter.burstCount; ++i)
                    spawnFromEmitter(registry, transform.x, transform.y, emitter);
            }
        }
    }

    // ────────────────────────────────────────
    // 2. Обновляем время жизни, цвет, размер
    //    (НЕ двигаем — движение в Scene::update/updateVisuals)
    // ────────────────────────────────────────
    auto particleView = registry.view<Particle, Sprite>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : particleView)
    {
        auto& particle = particleView.get<Particle>(entity);
        auto& sprite = particleView.get<Sprite>(entity);

        particle.elapsed += dt;

        if (particle.elapsed >= particle.lifetime)
        {
            toDestroy.push_back(entity);
            continue;
        }

        float t = particle.elapsed / particle.lifetime;

        // Размер
        sprite.width = particle.startWidth + (particle.endWidth - particle.startWidth) * t;
        sprite.height = particle.startHeight + (particle.endHeight - particle.startHeight) * t;

        // Цвет
        sprite.r = particle.startR + (particle.endR - particle.startR) * t;
        sprite.g = particle.startG + (particle.endG - particle.startG) * t;
        sprite.b = particle.startB + (particle.endB - particle.startB) * t;

        // Прозрачность
        if (particle.fadeOut)
            sprite.a = particle.startA + (particle.endA - particle.startA) * t;

        // Мерцание
        if (particle.flicker)
        {
            float flickerValue = 0.5f + 0.5f * std::sin(particle.elapsed * 20.0f);
            sprite.a *= flickerValue;
        }
    }

    // Уничтожаем мёртвые частицы
    for (auto entity : toDestroy)
        registry.destroy(entity);
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
    for (uint32_t i = 0; i < count; ++i)
        spawnFromEmitter(registry, x, y, emitter);
}

} // namespace wingz::ecs
