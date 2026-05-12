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
    float x, y;
    float sx = 1.0f, sy = 1.0f;
    float rot = 0.0f;
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 1.0f, v1 = 1.0f;
    uint32_t textureId = 0;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

/// Пакетный рендерер спрайтов.
/// Собирает спрайты в буфер и отрисовывает одним вызовом на текстуру.
class SpriteBatch
{
public:
    SpriteBatch();
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    /// Начинает новую партию.
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
