#include <stdexcept>

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "wingz/gfx/texture.h"

namespace wingz::gfx
{

struct Texture::Impl
{
    uint32_t handle = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

Texture::Texture(const std::string& path)
    : m_impl(std::make_unique<Impl>())
{
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data)
        throw std::runtime_error("Не удалось загрузить текстуру: " + path);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_impl->handle);
    glTextureStorage2D(m_impl->handle, 1, GL_RGBA8, w, h);
    glTextureSubImage2D(m_impl->handle, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTextureParameteri(m_impl->handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_impl->width = static_cast<uint32_t>(w);
    m_impl->height = static_cast<uint32_t>(h);

    stbi_image_free(data);
}

Texture::Texture(uint32_t width, uint32_t height, const uint8_t* rgba)
    : m_impl(std::make_unique<Impl>())
{
    glCreateTextures(GL_TEXTURE_2D, 1, &m_impl->handle);
    glTextureStorage2D(m_impl->handle, 1, GL_RGBA8, width, height);
    if (rgba)
        glTextureSubImage2D(m_impl->handle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    glTextureParameteri(m_impl->handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_impl->handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_impl->width = width;
    m_impl->height = height;
}

Texture::~Texture()
{
    if (m_impl->handle)
        glDeleteTextures(1, &m_impl->handle);
}

Texture::Texture(Texture&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_impl->handle)
            glDeleteTextures(1, &m_impl->handle);
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

void Texture::bind(uint32_t slot) const
{
    glBindTextureUnit(slot, m_impl->handle);
}

uint32_t Texture::handle() const
{
    return m_impl->handle;
}

uint32_t Texture::width() const
{
    return m_impl->width;
}

uint32_t Texture::height() const
{
    return m_impl->height;
}

} // namespace wingz::gfx
