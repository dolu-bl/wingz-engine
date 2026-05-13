#include <spdlog/spdlog.h>

#include "wingz/scene.h"

#include "wingz/ecs/components.h"
#include "wingz/ecs/particle.h"
#include "wingz/ecs/particle_system.h"
#include "wingz/ecs/systems.h"
#include "wingz/gfx/camera.h"
#include "wingz/gfx/sprite_batch.h"
#include "wingz/net/replication.h"
#include "wingz/physics/physics_world.h"

namespace wingz
{

struct Scene::Impl
{
    entt::registry registry;
    gfx::Camera camera;
    std::unique_ptr<physics::PhysicsWorld> physicsWorld;
    std::unique_ptr<ecs::ParticleSystem> particleSystem;
};

Scene::Scene()
    : m_impl(std::make_unique<Impl>())
{
    m_impl->physicsWorld = std::make_unique<physics::PhysicsWorld>();
    m_impl->particleSystem = std::make_unique<ecs::ParticleSystem>();
}

Scene::~Scene() = default;

void Scene::init()
{
    m_impl->camera.left = 0.0f;
    m_impl->camera.right = 1280.0f;
    m_impl->camera.bottom = 720.0f;
    m_impl->camera.top = 0.0f;

    auto& reg = m_impl->registry;

    // Резервируем сущности
    reg.storage<entt::entity>().reserve(4096);

    // Часто создаваемые/удаляемые компоненты — частицы, пули
    reg.storage<ecs::Transform>().reserve(2048);
    reg.storage<ecs::Velocity>().reserve(2048);
    reg.storage<ecs::Sprite>().reserve(2048);
    reg.storage<ecs::Particle>().reserve(2048);
    reg.storage<ecs::Tag>().reserve(2048);

    // Сетевые компоненты
    reg.storage<net::Networked>().reserve(256);
    reg.storage<ecs::Player>().reserve(256);
    reg.storage<ecs::InputIntent>().reserve(256);
    reg.storage<ecs::InterpolatedTransform>().reserve(256);

    // Физика
    reg.storage<physics::Collider>().reserve(512);

    // Редкие компоненты
    reg.storage<ecs::ParticleEmitter>().reserve(64);
}

void Scene::update(float dt)
{
    // 1. Ввод
    ecs::inputSystem(m_impl->registry);

    // 2. Движение (все сущности с Velocity)
    ecs::movementSystem(m_impl->registry, dt);

    // 3. Частицы (эмиттеры, время жизни, цвет, размер)
    m_impl->particleSystem->update(m_impl->registry, dt);

    // 4. Физика (коллизии)
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

void Scene::updateVisuals(float dt)
{
    // Двигаем только частицы (у них есть компонент Particle)
    auto moveView = m_impl->registry.view<ecs::Particle, ecs::Transform, ecs::Velocity>();
    for (auto entity : moveView)
    {
        auto& transform = moveView.get<ecs::Transform>(entity);
        auto& velocity = moveView.get<ecs::Velocity>(entity);

        transform.x += velocity.dx * dt;
        transform.y += velocity.dy * dt;
        transform.rot = std::atan2(velocity.dy, velocity.dx);

        // Затухание скорости
        velocity.dx *= 0.98f;
        velocity.dy *= 0.98f;
    }

    // Обновляем эмиттеры, время жизни, цвет, размер
    m_impl->particleSystem->update(m_impl->registry, dt);
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
