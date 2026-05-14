#include <stdexcept>

#include <spdlog/spdlog.h>

#include <wingz/core/asset_manager.h>

namespace wingz::core
{

gfx::Texture* AssetManager::loadTexture(
    const std::string& key,
    const std::string& path
)
{
    // Проверяем, нет ли уже загруженной
    auto it = m_textures.find(key);
    if (it != m_textures.end())
    {
        spdlog::debug("AssetManager: текстура '{}' уже загружена", key);
        return it->second.texture.get();
    }

    spdlog::info("AssetManager: загрузка текстуры '{}' из '{}'", key, path);

    auto texture = std::make_unique<gfx::Texture>(path);

    auto* ptr = texture.get();
    m_textures[key] = { std::move(texture), path };
    return ptr;
}

gfx::Texture* AssetManager::getTexture(const std::string& key) const
{
    auto it = m_textures.find(key);
    if (it != m_textures.end())
        return it->second.texture.get();
    return nullptr;
}

gfx::Texture* AssetManager::texture(
    const std::string& key,
    const std::string& path
)
{
    auto* tex = getTexture(key);
    if (tex)
        return tex;
    return loadTexture(key, path);
}

gfx::TextureAtlas* AssetManager::loadAtlas(
    const std::string& key,
    const std::string& imagePath,
    const std::string& jsonPath
)
{
    auto it = m_atlases.find(key);
    if (it != m_atlases.end())
    {
        spdlog::debug("AssetManager: атлас '{}' уже загружен", key);
        return it->second.atlas.get();
    }

    spdlog::info("AssetManager: загрузка атласа '{}' из '{}' + '{}'", key, imagePath, jsonPath);

    auto atlas = std::make_unique<gfx::TextureAtlas>(imagePath, jsonPath);

    auto* ptr = atlas.get();
    m_atlases[key] = { std::move(atlas), imagePath, jsonPath };
    return ptr;
}

gfx::TextureAtlas* AssetManager::getAtlas(const std::string& key) const
{
    auto it = m_atlases.find(key);
    if (it != m_atlases.end())
        return it->second.atlas.get();
    return nullptr;
}

void AssetManager::unload(const std::string& key)
{
    if (m_textures.erase(key))
    {
        spdlog::info("AssetManager: текстура '{}' выгружена", key);
        return;
    }
    if (m_atlases.erase(key))
    {
        spdlog::info("AssetManager: атлас '{}' выгружен", key);
        return;
    }
}

void AssetManager::clear()
{
    spdlog::info("AssetManager: выгрузка всех ресурсов (текстур: {}, атласов: {})", m_textures.size(), m_atlases.size());
    m_textures.clear();
    m_atlases.clear();
}

size_t AssetManager::textureCount() const
{
    return m_textures.size();
}

size_t AssetManager::atlasCount() const
{
    return m_atlases.size();
}

} // namespace wingz::core
