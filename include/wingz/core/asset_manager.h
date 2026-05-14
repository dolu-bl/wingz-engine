#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <wingz/gfx/texture.h>
#include <wingz/gfx/texture_atlas.h>

namespace wingz::core
{

/// Менеджер ресурсов.
/// Кеширует текстуры и атласы по строковому ключу.
/// Ленивая загрузка: ресурс загружается при первом запросе.
class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // ────────────────────────────────────────
    // Текстуры
    // ────────────────────────────────────────

    /// Загрузить текстуру (если уже загружена — вернёт кешированную).
    /// @param key Уникальный ключ (например, "player", "wall", "bg_cave").
    /// @param path Путь к файлу текстуры.
    gfx::Texture* loadTexture(const std::string& key, const std::string& path);

    /// Получить уже загруженную текстуру. nullptr если не найдена.
    gfx::Texture* getTexture(const std::string& key) const;

    /// Загрузить текстуру, если ещё не загружена.
    /// @param key Ключ.
    /// @param path Путь к файлу.
    gfx::Texture* texture(const std::string& key, const std::string& path);

    // ────────────────────────────────────────
    // Текстурные атласы
    // ────────────────────────────────────────

    /// Загрузить атлас (текстура + JSON с регионами).
    /// @param key Ключ (например, "tiles", "ui").
    /// @param imagePath Путь к PNG.
    /// @param jsonPath Путь к JSON с регионами.
    gfx::TextureAtlas* loadAtlas(
        const std::string& key,
        const std::string& imagePath,
        const std::string& jsonPath
    );

    /// Получить загруженный атлас.
    gfx::TextureAtlas* getAtlas(const std::string& key) const;

    // ────────────────────────────────────────
    // Управление
    // ────────────────────────────────────────

    /// Выгрузить ресурс по ключу.
    void unload(const std::string& key);

    /// Выгрузить все ресурсы.
    void clear();

    /// Количество загруженных ресурсов.
    size_t textureCount() const;
    size_t atlasCount() const;

private:
    struct TextureEntry
    {
        std::unique_ptr<gfx::Texture> texture;
        std::string path;
    };

    struct AtlasEntry
    {
        std::unique_ptr<gfx::TextureAtlas> atlas;
        std::string imagePath;
        std::string jsonPath;
    };

    std::unordered_map<std::string, TextureEntry> m_textures;
    std::unordered_map<std::string, AtlasEntry> m_atlases;
};

} // namespace wingz::core
