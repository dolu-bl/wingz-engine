#include <cmath>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include "wingz/physics/physics_world.h"

#include "wingz/ecs/components.h"

namespace wingz::physics
{

struct PhysicsWorld::Impl
{
    using CellKey = int64_t;

    static constexpr float kCellSize = 64.0f;

    static CellKey makeCellKey(int cx, int cy)
    {
        return (static_cast<int64_t>(cx) << 32) | static_cast<int64_t>(cy & 0xFFFFFFFF);
    }

    std::unordered_map<CellKey, std::vector<entt::entity>> buildGrid(entt::registry& registry)
    {
        std::unordered_map<CellKey, std::vector<entt::entity>> grid;

        auto view = registry.view<const ecs::Transform, const Collider>();
        for (auto entity : view)
        {
            const auto& transform = view.get<const ecs::Transform>(entity);
            const auto& collider = view.get<const Collider>(entity);

            float cx = transform.x + collider.box.x;
            float cy = transform.y + collider.box.y;

            // Добавляем сущность во все ячейки, которые она пересекает
            int minCellX = static_cast<int>(std::floor((cx - collider.box.halfW) / kCellSize));
            int maxCellX = static_cast<int>(std::floor((cx + collider.box.halfW) / kCellSize));
            int minCellY = static_cast<int>(std::floor((cy - collider.box.halfH) / kCellSize));
            int maxCellY = static_cast<int>(std::floor((cy + collider.box.halfH) / kCellSize));

            for (int cy2 = minCellY; cy2 <= maxCellY; ++cy2)
            {
                for (int cx2 = minCellX; cx2 <= maxCellX; ++cx2)
                {
                    grid[makeCellKey(cx2, cy2)].push_back(entity);
                }
            }
        }

        return grid;
    }
};

PhysicsWorld::PhysicsWorld()
    : m_impl(std::make_unique<Impl>())
{
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::update(
    entt::registry& registry,
    float dt,
    CollisionCallback callback
)
{
    using CellKey = Impl::CellKey;

    auto grid = m_impl->buildGrid(registry);
    auto view = registry.view<const ecs::Transform, const Collider>();

    // Множество уже проверенных пар (чтобы не дублировать)
    std::unordered_set<uint64_t> checkedPairs;

    // Обрабатываем коллизии
    for (const auto& [cellKey, entities] : grid)
    {
        for (size_t i = 0; i < entities.size(); ++i)
        {
            entt::entity entityA = entities[i];
            if (!registry.valid(entityA))
                continue;

            // Проверяем все остальные сущности в этой же ячейке
            for (size_t j = i + 1; j < entities.size(); ++j)
            {
                entt::entity entityB = entities[j];
                if (!registry.valid(entityB))
                    continue;

                // Уникальный ключ пары
                const uint64_t pairKey = (static_cast<uint64_t>(static_cast<uint32_t>(entityA)) << 32)
                    | static_cast<uint64_t>(static_cast<uint32_t>(entityB));
                if (checkedPairs.count(pairKey))
                    continue;
                checkedPairs.insert(pairKey);

                const auto& transformA = view.get<const ecs::Transform>(entityA);
                const auto& colliderA = view.get<const Collider>(entityA);
                const auto& transformB = view.get<const ecs::Transform>(entityB);
                const auto& colliderB = view.get<const Collider>(entityB);

                // Проверяем маски слоёв
                if (!Collider::layersOverlap(colliderA.mask, colliderB.layer)
                    && !Collider::layersOverlap(colliderB.mask, colliderA.layer))
                {
                    continue;
                }

                // Вычисляем мировые AABB
                const AABB worldA = {
                    transformA.x + colliderA.box.x,
                    transformA.y + colliderA.box.y,
                    colliderA.box.halfW,
                    colliderA.box.halfH
                };
                const AABB worldB = {
                    transformB.x + colliderB.box.x,
                    transformB.y + colliderB.box.y,
                    colliderB.box.halfW,
                    colliderB.box.halfH
                };

                if (!Collider::overlap(worldA, worldB))
                    continue;

                // Колбэк
                if (callback)
                {
                    CollisionEvent event;
                    event.entityA = entityA;
                    event.entityB = entityB;
                    event.layerA = colliderA.layer;
                    event.layerB = colliderB.layer;
                    callback(event);
                }

                // Разрешение коллизий (выталкивание)
                if (colliderA.isTrigger || colliderB.isTrigger)
                    continue;

                const float overlapX = worldA.halfW + worldB.halfW - std::abs(worldA.x - worldB.x);
                const float overlapY = worldA.halfH + worldB.halfH - std::abs(worldA.y - worldB.y);

                if (overlapX <= 0.0f || overlapY <= 0.0f)
                    continue;

                // Определяем, кого выталкивать
                const bool pushA = !colliderA.isStatic;
                const bool pushB = !colliderB.isStatic;

                if (!pushA && !pushB)
                    continue;

                const float totalMass = (pushA ? 1.0f : 0.0f) + (pushB ? 1.0f : 0.0f);
                const float ratioA = pushA ? 1.0f / totalMass : 0.0f;
                const float ratioB = pushB ? 1.0f / totalMass : 0.0f;

                // Выталкиваем по минимальной оси перекрытия
                if (overlapX < overlapY)
                {
                    const float sign = (worldA.x < worldB.x) ? -1.0f : 1.0f;
                    if (pushA)
                    {
                        auto* ta = registry.try_get<ecs::Transform>(entityA);
                        if (ta)
                            ta->x += sign * overlapX * ratioA;
                    }
                    if (pushB)
                    {
                        auto* tb = registry.try_get<ecs::Transform>(entityB);
                        if (tb)
                            tb->x -= sign * overlapX * ratioB;
                    }
                }
                else
                {
                    const float sign = (worldA.y < worldB.y) ? -1.0f : 1.0f;
                    if (pushA)
                    {
                        auto* ta = registry.try_get<ecs::Transform>(entityA);
                        if (ta)
                            ta->y += sign * overlapY * ratioA;
                    }
                    if (pushB)
                    {
                        auto* tb = registry.try_get<ecs::Transform>(entityB);
                        if (tb)
                            tb->y -= sign * overlapY * ratioB;
                    }
                }
            }
        }
    }
}

} // namespace wingz::physics
