#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <wingz/gfx/texture.h>

namespace wingz::core
{

using ResourceId = uint32_t;
constexpr ResourceId kInvalidResourceId = 0;

/// Информация о загруженном ресурсе (текстура или регион атласа)
struct ResourceInfo
{
    ResourceId id = kInvalidResourceId;
    std::string name;

    // Данные для рендера
    uint32_t textureHandle = 0;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    float width = 0.0f;
    float height = 0.0f;

    // Отладка
    std::string debugName;
};

/// Менеджер ресурсов — загружает всё из единого манифеста
class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /// Загружает манифест и все описанные в нём ресурсы
    /// @param manifestPath Путь к JSON-файлу манифеста
    bool loadManifest(const std::string& manifestPath);

    /// Получить информацию о ресурсе по ID
    const ResourceInfo* getResource(ResourceId id) const;

    /// Получить информацию о ресурсе по имени
    const ResourceInfo* getResource(const std::string& name) const;

    /// Получить ID ресурса по имени
    ResourceId getResourceId(const std::string& name) const;

    /// Выгрузить ресурс
    void unload(ResourceId id);

    /// Выгрузить все ресурсы
    void clear();

    /// Количество загруженных ресурсов
    size_t resourceCount() const { return m_resourcesById.size(); }

private:
    struct AtlasTexture
    {
        std::unique_ptr<gfx::Texture> texture;
        uint32_t handle = 0;
        float width = 0.0f, height = 0.0f;
    };

    // Загружает текстуру атласа и все его регионы
    bool loadAtlas(const std::string& name, const nlohmann::json& atlasData);

    // Загружает отдельную текстуру
    bool loadSeparateTexture(
        const std::string& name,
        const nlohmann::json& textureData
    );

    // Регистрирует ресурс во внутренних индексах
    void registerResource(
        ResourceId id,
        const std::string& name,
        uint32_t textureHandle,
        float u0,
        float v0,
        float u1,
        float v1,
        float width,
        float height
    );

    std::unordered_map<ResourceId, ResourceInfo> m_resourcesById;
    std::unordered_map<std::string, ResourceId> m_resourcesByName;

    // Хранилища текстур (владеют памятью)
    std::unordered_map<std::string, AtlasTexture> m_atlases;
    std::unordered_map<ResourceId, std::unique_ptr<gfx::Texture>> m_separateTextures;
};

} // namespace wingz::core
