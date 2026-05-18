#pragma once

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

#include <wingz/core/asset_manager.h>
#include <wingz/ecs/components.h>

namespace wingz::ecs
{

/// Система обновления анимаций.
/// Обновляет Sprite компонент согласно текущему кадру Animator.
inline void animationSystem(
    entt::registry& registry,
    core::AssetManager* assets,
    float dt
)
{
    if (!assets)
        return;

    auto view = registry.view<Animator, Sprite>();

    for (auto entity : view)
    {
        auto& anim = view.get<Animator>(entity);
        auto& sprite = view.get<Sprite>(entity);

        // Анимация не играет или нет кадров — пропускаем
        if (!anim.playing || anim.frames.empty())
            continue;

        // Обрабатываем задержку перед стартом
        if (anim.startDelay > 0.0f && anim.delayElapsed < anim.startDelay)
        {
            anim.delayElapsed += dt;
            continue;
        }

        // Увеличиваем время в текущем кадре
        anim.elapsed += dt;

        // Получаем текущий кадр
        const auto& currentFrame = anim.frames[anim.currentFrame];

        // Проверяем, нужно ли переключиться на следующий кадр
        if (anim.elapsed >= currentFrame.duration)
        {
            anim.elapsed = 0.0f;
            anim.currentFrame++;

            // Дошли до конца
            if (anim.currentFrame >= anim.frames.size())
            {
                if (anim.looping)
                {
                    anim.currentFrame = 0; // Начинаем сначала
                }
                else
                {
                    anim.playing = false; // Останавливаемся
                    anim.currentFrame = anim.frames.size() - 1; // Остаёмся на последнем кадре
                }
            }
        }

        // Если анимация остановилась и не зациклена — ничего не обновляем
        if (!anim.playing && !anim.looping)
            continue;

        // Берём актуальный кадр (возможно, после переключения)
        const auto& frame = anim.frames[anim.currentFrame];

        // Получаем ресурс из AssetManager
        const auto* resource = assets->getResource(frame.resourceId);
        if (!resource)
        {
            spdlog::warn("animationSystem: ресурс {} не найден", frame.resourceId);
            continue;
        }

        // Обновляем спрайт
        sprite.textureId = resource->textureHandle;
        sprite.width = resource->width * anim.scale;
        sprite.height = resource->height * anim.scale;

        // Применяем отражение, если нужно
        if (anim.flipX)
        {
            sprite.u0 = resource->u1;
            sprite.u1 = resource->u0;
        }
        else
        {
            sprite.u0 = resource->u0;
            sprite.u1 = resource->u1;
        }

        if (anim.flipY)
        {
            sprite.v0 = resource->v1;
            sprite.v1 = resource->v0;
        }
        else
        {
            sprite.v0 = resource->v0;
            sprite.v1 = resource->v1;
        }
    }
}

} // namespace wingz::ecs
