#pragma once

#include <memory>
#include <vector>

#include <entt/entt.hpp>

#include <wingz/physics/collider.h>

namespace wingz::physics
{

/// Простой 2D-физический мир.
/// Использует пространственную сетку для быстрого поиска коллизий.
class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    /// Шаг симуляции. Вызывает колбэк для каждой найденной коллизии.
    void update(
        entt::registry& registry,
        float dt,
        CollisionCallback callback = nullptr
    );

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::physics
