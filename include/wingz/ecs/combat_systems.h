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
