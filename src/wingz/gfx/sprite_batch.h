#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace wingz::gfx
{

struct Camera;
class Texture;

/// Описание одного спрайта для отрисовки.
struct SpriteDesc
{
    float x, y; // позиция центра в мировых координатах
    float sx = 1.0f, sy = 1.0f; // масштаб
    float rot = 0.0f; // поворот в радианах
    float u0 = 0.0f, v0 = 0.0f; // текстурные координаты
    float u1 = 1.0f, v1 = 1.0f;
    uint32_t textureId = 0; // идентификатор текстуры (0 — белая заглушка)
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f; // tint
};

/// Пакетный рендерер спрайтов.
/// Собирает спрайты в буфер и отрисовывает одним draw call.
class SpriteBatch
{
public:
    SpriteBatch();
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    /// Начинает новую партию. Вызывается один раз за кадр.
    void begin(const Camera& camera);

    /// Добавляет спрайт в очередь отрисовки.
    void draw(const SpriteDesc& desc);

    /// Отрисовывает все накопленные спрайты.
    void end();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
