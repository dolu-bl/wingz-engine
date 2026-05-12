#pragma once

#include <entt/entt.hpp>

#include <wingz/ecs/components.h>

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

/// Система ввода: преобразует InputIntent в Velocity.
inline void inputSystem(entt::registry& registry)
{
    auto view = registry.view<Velocity, const InputIntent>();
    for (auto entity : view)
    {
        auto& velocity = view.get<Velocity>(entity);
        const auto& intent = view.get<const InputIntent>(entity);

        constexpr float kSpeed = 300.0f;
        velocity.dx = intent.moveX * kSpeed;
        velocity.dy = intent.moveY * kSpeed;
    }
}

} // namespace wingz::ecs
