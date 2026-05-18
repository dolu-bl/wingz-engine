#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "wingz/gfx/texture.h"
#include "wingz/gfx/texture_atlas.h"

namespace wingz::gfx
{

struct TextureAtlas::Impl
{
    std::unique_ptr<Texture> texture;
    std::unordered_map<std::string, AtlasRegion> regions;
};

TextureAtlas::TextureAtlas(const std::string& imagePath, const std::string& jsonPath)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->texture = std::make_unique<Texture>(imagePath);

    const float textureWidth = static_cast<float>(m_impl->texture->width());
    const float textureHeight = static_cast<float>(m_impl->texture->height());

    std::ifstream file(jsonPath);
    if (!file)
        throw std::runtime_error("Не удалось открыть JSON-атлас: " + jsonPath);

    nlohmann::json json;
    file >> json;

    for (const auto& [name, region] : json.items())
    {
        AtlasRegion reg;
        reg.textureId = m_impl->texture->handle();
        reg.width = region.value("width", 0.0f);
        reg.height = region.value("height", 0.0f);

        float x = region.value("x", 0.0f);
        float y = region.value("y", 0.0f);

        // БЕЗ ИНВЕРСИИ — просто прямые координаты
        reg.u0 = x / textureWidth;
        reg.v0 = y / textureHeight;
        reg.u1 = (x + reg.width) / textureWidth;
        reg.v1 = (y + reg.height) / textureHeight;

        m_impl->regions[name] = reg;
    }
}

TextureAtlas::~TextureAtlas() = default;

const AtlasRegion* TextureAtlas::find(const std::string& name) const
{
    auto it = m_impl->regions.find(name);
    if (it != m_impl->regions.end())
        return &it->second;
    return nullptr;
}

const Texture& TextureAtlas::texture() const
{
    return *m_impl->texture;
}

} // namespace wingz::gfx
