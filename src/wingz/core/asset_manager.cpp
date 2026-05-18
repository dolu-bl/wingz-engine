#include <fstream>

#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <wingz/core/asset_manager.h>
#include <wingz/gfx/texture.h>

namespace wingz::core
{

bool AssetManager::loadManifest(const std::string& manifestPath)
{
    std::ifstream file(manifestPath);
    if (!file)
    {
        spdlog::error("AssetManager: не удалось открыть манифест: {}", manifestPath);
        return false;
    }

    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (const std::exception& e)
    {
        spdlog::error("AssetManager: ошибка парсинга манифеста: {}", e.what());
        return false;
    }

    // Загружаем атласы
    if (json.contains("atlases"))
    {
        for (const auto& [name, data] : json["atlases"].items())
        {
            if (!loadAtlas(name, data))
            {
                spdlog::error("AssetManager: не удалось загрузить атлас '{}'", name);
                return false;
            }
        }
    }

    // Загружаем отдельные текстуры
    if (json.contains("textures"))
    {
        for (const auto& [name, data] : json["textures"].items())
        {
            if (!loadSeparateTexture(name, data))
            {
                spdlog::error("AssetManager: не удалось загрузить текстуру '{}'", name);
                return false;
            }
        }
    }

    spdlog::info("AssetManager: манифест загружен, ресурсов: {}", m_resourcesById.size());
    return true;
}

bool AssetManager::loadAtlas(const std::string& name, const nlohmann::json& atlasData)
{
    std::string imagePath = atlasData.value("image", "");
    if (imagePath.empty())
    {
        spdlog::error("AssetManager: атлас '{}' не имеет поля 'image'", name);
        return false;
    }

    if (!atlasData.contains("regions"))
    {
        spdlog::error("AssetManager: атлас '{}' не имеет поля 'regions'", name);
        return false;
    }

    // Загружаем текстуру атласа
    auto atlasTexture = std::make_unique<gfx::Texture>(imagePath);
    uint32_t atlasHandle = atlasTexture->handle();
    float atlasWidth = static_cast<float>(atlasTexture->width());
    float atlasHeight = static_cast<float>(atlasTexture->height());

    // Сохраняем атлас
    AtlasTexture stored;
    stored.texture = std::move(atlasTexture);
    stored.handle = atlasHandle;
    stored.width = atlasWidth;
    stored.height = atlasHeight;
    m_atlases[name] = std::move(stored);

    // Регистрируем каждый регион
    for (const auto& [regionName, regionData] : atlasData["regions"].items())
    {
        ResourceId id = regionData.value("id", kInvalidResourceId);
        if (id == kInvalidResourceId)
        {
            spdlog::error("AssetManager: регион '{}/{}' не имеет ID", name, regionName);
            return false;
        }

        float x = regionData.value("x", 0.0f);
        float y = regionData.value("y", 0.0f);
        float width = regionData.value("width", 0.0f);
        float height = regionData.value("height", 0.0f);

        if (width <= 0.0f || height <= 0.0f)
        {
            spdlog::error("AssetManager: регион '{}/{}' имеет нулевые размеры", name, regionName);
            return false;
        }

        // Вычисляем текстурные координаты
        float u0 = x / atlasWidth;
        float v0 = y / atlasHeight; // Прямые координаты из JSON
        float u1 = (x + width) / atlasWidth;
        float v1 = (y + height) / atlasHeight;

        // ИНВЕРТИРУЕМ V координаты для OpenGL (Y=0 = низ текстуры)
        float invV0 = 1.0f - v1; // меняем местами v0 и v1 при инверсии
        float invV1 = 1.0f - v0;

        // Полное имя ресурса = "атлас:регион"
        std::string fullName = name + ":" + regionName;

        // Регистрируем с ИНВЕРТИРОВАННЫМИ координатами
        registerResource(id, fullName, atlasHandle, u0, invV0, u1, invV1, width, height);

        // Также регистрируем короткое имя (если ещё не занято)
        if (m_resourcesByName.find(regionName) == m_resourcesByName.end())
        {
            registerResource(id, regionName, atlasHandle, u0, invV0, u1, invV1, width, height);
        }

        spdlog::debug("AssetManager: зарегистрирован регион '{}' (ID {})", fullName, id);
    }

    return true;
}

bool AssetManager::loadSeparateTexture(const std::string& name, const nlohmann::json& textureData)
{
    ResourceId id = textureData.value("id", kInvalidResourceId);
    if (id == kInvalidResourceId)
    {
        spdlog::error("AssetManager: текстура '{}' не имеет ID", name);
        return false;
    }

    std::string path = textureData.value("path", "");
    if (path.empty())
    {
        spdlog::error("AssetManager: текстура '{}' не имеет поля 'path'", name);
        return false;
    }

    auto texture = std::make_unique<gfx::Texture>(path);
    uint32_t handle = texture->handle();
    float width = static_cast<float>(texture->width());
    float height = static_cast<float>(texture->height());

    // Сохраняем текстуру
    m_separateTextures[id] = std::move(texture);

    // Регистрируем ресурс (вся текстура целиком)
    registerResource(id, name, handle, 0.0f, 0.0f, 1.0f, 1.0f, width, height);

    spdlog::debug("AssetManager: зарегистрирована текстура '{}' (ID {})", name, id);
    return true;
}

void AssetManager::registerResource(
    ResourceId id,
    const std::string& name,
    uint32_t textureHandle,
    float u0,
    float v0,
    float u1,
    float v1,
    float width,
    float height
)
{
    ResourceInfo info;
    info.id = id;
    info.name = name;
    info.textureHandle = textureHandle;
    info.u0 = u0;
    info.v0 = v0;
    info.u1 = u1;
    info.v1 = v1;
    info.width = width;
    info.height = height;
    info.debugName = name;

    m_resourcesById[id] = info;
    m_resourcesByName[name] = id;
}

const ResourceInfo* AssetManager::getResource(ResourceId id) const
{
    auto it = m_resourcesById.find(id);
    return it != m_resourcesById.end() ? &it->second : nullptr;
}

const ResourceInfo* AssetManager::getResource(const std::string& name) const
{
    auto it = m_resourcesByName.find(name);
    if (it == m_resourcesByName.end())
        return nullptr;
    return getResource(it->second);
}

ResourceId AssetManager::getResourceId(const std::string& name) const
{
    auto it = m_resourcesByName.find(name);
    return it != m_resourcesByName.end() ? it->second : kInvalidResourceId;
}

void AssetManager::unload(ResourceId id)
{
    auto it = m_resourcesById.find(id);
    if (it == m_resourcesById.end())
        return;

    std::string name = it->second.name;
    m_resourcesById.erase(it);
    m_resourcesByName.erase(name);

    // Удаляем из отдельных текстур, если там есть
    m_separateTextures.erase(id);

    spdlog::debug("AssetManager: выгружен ресурс ID {}", id);
}

void AssetManager::clear()
{
    m_resourcesById.clear();
    m_resourcesByName.clear();
    m_atlases.clear();
    m_separateTextures.clear();
    spdlog::info("AssetManager: все ресурсы выгружены");
}

} // namespace wingz::core
