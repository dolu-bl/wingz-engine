#include "wingz/scene.h"

#include "wingz/ecs/components.h"
#include "wingz/ecs/systems.h"
#include "wingz/gfx/camera.h"
#include "wingz/gfx/sprite_batch.h"

namespace wingz
{

struct Scene::Impl
{
    entt::registry registry;
    gfx::Camera camera;
};

Scene::Scene()
    : m_impl(std::make_unique<Impl>())
{
}

Scene::~Scene() = default;

void Scene::init()
{
    // Настраиваем камеру (будет переопределено при ресайзе)
    m_impl->camera.left = 0.0f;
    m_impl->camera.right = 1280.0f;
    m_impl->camera.bottom = 720.0f;
    m_impl->camera.top = 0.0f;
}

void Scene::update(float dt)
{
    ecs::movementSystem(m_impl->registry, dt);
}

void Scene::render(gfx::SpriteBatch& batch)
{
    batch.begin(m_impl->camera);

    auto view = m_impl->registry.view<const ecs::Transform, const ecs::Sprite>();
    for (auto entity : view)
    {
        const auto& transform = view.get<const ecs::Transform>(entity);
        const auto& sprite = view.get<const ecs::Sprite>(entity);

        gfx::SpriteDesc desc;
        desc.x = transform.x;
        desc.y = transform.y;
        desc.sx = sprite.width;
        desc.sy = sprite.height;
        desc.rot = transform.rot;
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
