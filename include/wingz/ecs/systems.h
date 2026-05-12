#pragma once

#include <entt/entt.hpp>

#include "components.h"

namespace wingz::ecs
{

/// Система движения: обновляет Transform согласно Velocity.
inline void movementSystem(entt::registry& registry, float dt)
{
    auto view = registry.view<Transform, const Velocity>();
    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        const auto& velocity = view.get<const Velocity>(entity);

        transform.x += velocity.dx * dt;
        transform.y += velocity.dy * dt;
        transform.rot += velocity.drot * dt;
    }
}

} // namespace wingz::ecs
