#pragma once

#include <vector>

#include <entt/entt.hpp>

#include <wingz/ecs/components.h>
#include <wingz/ecs/particle.h>
#include <wingz/ecs/particle_system.h>
#include <wingz/physics/collider.h>

namespace wingz::ecs
{

/// Система урона: обрабатывает коллизии пуль со стенами/игроками.
inline void damageSystem(
    entt::registry& registry,
    const physics::CollisionEvent& event
)
{
    auto* bulletA = registry.try_get<Bullet>(event.entityA);
    auto* bulletB = registry.try_get<Bullet>(event.entityB);

    entt::entity bulletEntity = entt::null;
    entt::entity targetEntity = entt::null;
    const Bullet* bullet = nullptr;

    if (bulletA && !bulletB)
    {
        bulletEntity = event.entityA;
        targetEntity = event.entityB;
        bullet = bulletA;
    }
    else if (bulletB && !bulletA)
    {
        bulletEntity = event.entityB;
        targetEntity = event.entityA;
        bullet = bulletB;
    }
    else
    {
        return;
    }

    auto* health = registry.try_get<Health>(targetEntity);
    if (health)
    {
        health->current -= bullet->damage;

        const auto* targetTransform = registry.try_get<Transform>(targetEntity);
        if (targetTransform)
        {
            ParticleEmitter debrisEmitter;
            debrisEmitter.baseLifetime = 0.4f;
            debrisEmitter.lifetimeVariance = 0.2f;
            debrisEmitter.baseSpeed = 150.0f;
            debrisEmitter.speedVariance = 80.0f;
            debrisEmitter.spreadAngle = 3.14159265f;
            debrisEmitter.baseAngle = 0.0f;
            debrisEmitter.startR = 0.8f;
            debrisEmitter.startG = 0.3f;
            debrisEmitter.startB = 0.3f;
            debrisEmitter.startA = 1.0f;
            debrisEmitter.endR = 0.6f;
            debrisEmitter.endG = 0.2f;
            debrisEmitter.endB = 0.2f;
            debrisEmitter.endA = 0.0f;
            debrisEmitter.startWidth = 6.0f;
            debrisEmitter.startHeight = 6.0f;
            debrisEmitter.endWidth = 2.0f;
            debrisEmitter.endHeight = 2.0f;
            debrisEmitter.fadeOut = true;
            debrisEmitter.flicker = true;
            debrisEmitter.particleType = Particle::Type::Spark;

            ParticleSystem::emitBurst(
                registry,
                targetTransform->x, targetTransform->y,
                debrisEmitter,
                12
            );
        }
    }

    registry.destroy(bulletEntity);
}

/// Система смерти: помечает сущности с Health <= 0 и уничтожает по таймеру.
inline void deathSystem(entt::registry& registry, float dt)
{
    // Помечаем мёртвых
    auto healthView = registry.view<Health>();
    for (auto entity : healthView)
    {
        auto& health = healthView.get<Health>(entity);
        if (health.current <= 0.0f)
        {
            if (!registry.try_get<Dead>(entity))
                registry.emplace<Dead>(entity, 0.5f);
        }
    }

    // Удаляем по таймеру
    auto deadView = registry.view<Dead>();
    std::vector<entt::entity> toDestroy;

    for (auto entity : deadView)
    {
        auto& dead = deadView.get<Dead>(entity);
        dead.timer -= dt;
        if (dead.timer <= 0.0f)
            toDestroy.push_back(entity);
    }

    for (auto entity : toDestroy)
        registry.destroy(entity);
}

} // namespace wingz::ecs
