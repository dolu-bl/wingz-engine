#pragma once

#include <memory>

#include <entt/entt.hpp>

#include <wingz/net/types.h>

namespace wingz
{

namespace gfx
{
class SpriteBatch;
class Camera;
}
namespace ecs
{
class ParticleSystem;
}

/// Контейнер для ECS-мира и систем.
/// Владеет registry и предоставляет доступ к нему.
class Scene
{
public:
    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    /// Инициализация после создания окна и графики.
    void init();

    /// Обновление логики.
    void update(float dt);

    /// Обновление только визуальных систем (частицы).
    /// Вызывается на клиенте каждый кадр.
    void updateVisuals(float dt);

    /// Отрисовка.
    void render(gfx::SpriteBatch& batch);

    entt::registry& registry();

    /// Установить коллбэк при попадании пули (для сети).
    void setHitCallback(std::function<void(float x, float y)> callback);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz
