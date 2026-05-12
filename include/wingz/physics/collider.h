#pragma once

#include <cstdint>
#include <functional>

#include <entt/entt.hpp>

namespace wingz::physics
{

/// Тип коллизии.
enum class CollisionLayer : uint8_t
{
    None = 0,
    Player = 1 << 0,
    Enemy = 1 << 1,
    PlayerBullet = 1 << 2,
    EnemyBullet = 1 << 3,
    Wall = 1 << 4,
    Pickup = 1 << 5,
    All = 0xFF
};

/// Побитовые операторы для CollisionLayer.
inline CollisionLayer operator&(CollisionLayer a, CollisionLayer b)
{
    return static_cast<CollisionLayer>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}

inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b)
{
    return static_cast<CollisionLayer>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

/// Прямоугольный коллайдер (AABB).
struct AABB
{
    float x, y;
    float halfW, halfH;
};

/// Компонент коллайдера для ECS.
struct Collider
{
    AABB box;
    CollisionLayer layer = CollisionLayer::None;
    CollisionLayer mask = CollisionLayer::None;
    bool isTrigger = false;
    bool isStatic = false;

    static bool layersOverlap(CollisionLayer a, CollisionLayer b)
    {
        return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
    }

    static bool overlap(const AABB& a, const AABB& b)
    {
        return (std::abs(a.x - b.x) < a.halfW + b.halfW) && (std::abs(a.y - b.y) < a.halfH + b.halfH);
    }
};

/// Событие коллизии.
struct CollisionEvent
{
    entt::entity entityA;
    entt::entity entityB;
    CollisionLayer layerA;
    CollisionLayer layerB;
};

/// Колбэк при коллизии.
using CollisionCallback = std::function<void(const CollisionEvent&)>;

} // namespace wingz::physics
