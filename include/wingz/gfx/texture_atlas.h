#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace wingz::gfx
{

class Texture;

/// Прямоугольник внутри атласа.
struct AtlasRegion
{
    uint32_t textureId; // хендл OpenGL-текстуры
    float u0, v0; // левый верхний угол
    float u1, v1; // правый нижний угол
    float width, height; // размеры в пикселях
};

/// Текстурный атлас.
/// Загружает большую текстуру и позволяет нарезать её на поименованные регионы.
class TextureAtlas
{
public:
    /// Загружает текстуру и JSON-описание регионов.
    TextureAtlas(const std::string& imagePath, const std::string& jsonPath);
    ~TextureAtlas();

    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    /// Возвращает регион по имени субтекстуры.
    const AtlasRegion* find(const std::string& name) const;

    /// Базовая текстура атласа.
    const Texture& texture() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
