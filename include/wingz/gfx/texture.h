#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace wingz::gfx
{

/// 2D-текстура, хранящаяся на GPU.
class Texture
{
public:
    /// Загружает текстуру из файла (PNG, JPG, BMP через stb_image).
    explicit Texture(const std::string& path);

    /// Создаёт текстуру заданного размера с сырыми пикселями.
    Texture(uint32_t width, uint32_t height, const uint8_t* rgba);

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /// Привязывает текстуру к текстурному слоту.
    void bind(uint32_t slot) const;

    uint32_t handle() const;
    uint32_t width() const;
    uint32_t height() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
