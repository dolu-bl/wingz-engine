#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "wingz/gfx/texture_atlas.h"
#include "wingz/gfx/texture.h"

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

    float texW = static_cast<float>(m_impl->texture->width());
    float texH = static_cast<float>(m_impl->texture->height());

    // Загружаем JSON с регионами
    std::ifstream file(jsonPath);
    if (!file)
        throw std::runtime_error("Не удалось открыть JSON-атлас: " + jsonPath);

    nlohmann::json json;
    file >> json;

    for (const auto& [name, region] : json.items())
    {
        AtlasRegion r;
        r.textureId = m_impl->texture->handle();
        r.width = region.value("width", 0.0f);
        r.height = region.value("height", 0.0f);
        float x = region.value("x", 0.0f);
        float y = region.value("y", 0.0f);

        r.u0 = x / texW;
        r.v0 = y / texH;
        r.u1 = (x + r.width) / texW;
        r.v1 = (y + r.height) / texH;

        m_impl->regions[name] = r;
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
