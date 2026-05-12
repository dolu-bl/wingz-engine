#include <spdlog/spdlog.h>

#include "wingz/scene.h"

#include "wingz/ecs/components.h"
#include "wingz/ecs/systems.h"
#include "wingz/gfx/camera.h"
#include "wingz/gfx/sprite_batch.h"
#include "wingz/physics/physics_world.h"

namespace wingz
{

struct Scene::Impl
{
    entt::registry registry;
    gfx::Camera camera;
    std::unique_ptr<physics::PhysicsWorld> physicsWorld;
};

Scene::Scene()
    : m_impl(std::make_unique<Impl>())
{
    m_impl->physicsWorld = std::make_unique<physics::PhysicsWorld>();
}

Scene::~Scene() = default;

void Scene::init()
{
    m_impl->camera.left = 0.0f;
    m_impl->camera.right = 1280.0f;
    m_impl->camera.bottom = 720.0f;
    m_impl->camera.top = 0.0f;
}

void Scene::update(float dt)
{
    // 1. Ввод
    ecs::inputSystem(m_impl->registry);

    // 2. Движение
    ecs::movementSystem(m_impl->registry, dt);

    // 3. Физика (коллизии)
    m_impl->physicsWorld->update(
        m_impl->registry,
        dt,
        [](const physics::CollisionEvent& event)
        {
            spdlog::debug(
                "Коллизия: entity {} vs entity {}",
                static_cast<uint32_t>(event.entityA),
                static_cast<uint32_t>(event.entityB)
            );
        }
    );
}

void Scene::render(gfx::SpriteBatch& batch)
{
    batch.begin(m_impl->camera);

    auto view = m_impl->registry.view<const ecs::Transform, const ecs::Sprite>();
    for (auto entity : view)
    {
        const auto& transform = view.get<const ecs::Transform>(entity);
        const auto& sprite = view.get<const ecs::Sprite>(entity);

        // Используем интерполированную позицию, если есть
        float renderX = transform.x;
        float renderY = transform.y;
        float renderRot = transform.rot;

        const auto* itransform = m_impl->registry.try_get<ecs::InterpolatedTransform>(entity);
        if (itransform)
        {
            renderX = itransform->x;
            renderY = itransform->y;
            renderRot = itransform->rot;
        }

        gfx::SpriteDesc desc;
        desc.x = renderX;
        desc.y = renderY;
        desc.sx = sprite.width;
        desc.sy = sprite.height;
        desc.rot = renderRot;
        desc.u0 = sprite.u0;
        desc.v0 = sprite.v0;
        desc.u1 = sprite.u1;
        desc.v1 = sprite.v1;
        desc.textureId = sprite.textureId;
        desc.r = sprite.r;
        desc.g = sprite.g;
        desc.b = sprite.b;
        desc.a = sprite.a;

        batch.draw(desc);
    }

    batch.end();
}

entt::registry& Scene::registry()
{
    return m_impl->registry;
}

} // namespace wingz
