#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

#include <glad/glad.h>

#include "wingz/gfx/camera.h"
#include "wingz/gfx/shader.h"
#include "wingz/gfx/sprite_batch.h"

namespace wingz::gfx
{

// Встроенные шейдеры
namespace
{

const char* kVertexShader = R"(
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

const char* kFragmentShader = R"(
#version 460 core
in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    FragColor = texture(uTexture, vTexCoord) * vColor;
}
)";

} // namespace

struct SpriteBatch::Impl
{
    std::unique_ptr<ShaderProgram> shader;
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t whiteTexture = 0; // белая текстура 1x1 для спрайтов без текстуры

    static constexpr size_t kMaxSprites = 10000;
    static constexpr size_t kVerticesPerSprite = 4;
    static constexpr size_t kIndicesPerSprite = 6;

    struct Vertex
    {
        float x, y;
        float u, v;
        float r, g, b, a;
    };

    // Группируем спрайты по текстуре
    struct SpriteGroup
    {
        uint32_t textureId;
        std::vector<Vertex> vertices;
    };

    std::vector<SpriteDesc> sprites;
    Camera currentCamera;

    void initWhiteTexture()
    {
        uint8_t white[] = { 255, 255, 255, 255 };
        glCreateTextures(GL_TEXTURE_2D, 1, &whiteTexture);
        glTextureStorage2D(whiteTexture, 1, GL_RGBA8, 1, 1);
        glTextureSubImage2D(whiteTexture, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTextureParameteri(whiteTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(whiteTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
};

SpriteBatch::SpriteBatch()
    : m_impl(std::make_unique<Impl>())
{
    // Создаём OpenGL-объекты
    glCreateVertexArrays(1, &m_impl->vao);
    glCreateBuffers(1, &m_impl->vbo);
    glCreateBuffers(1, &m_impl->ebo);

    // Настраиваем VAO
    // Vertex buffer
    glVertexArrayVertexBuffer(m_impl->vao, 0, m_impl->vbo, 0, sizeof(Impl::Vertex));
    glVertexArrayAttribFormat(m_impl->vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Impl::Vertex, x));
    glVertexArrayAttribFormat(m_impl->vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Impl::Vertex, u));
    glVertexArrayAttribFormat(m_impl->vao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(Impl::Vertex, r));
    glEnableVertexArrayAttrib(m_impl->vao, 0);
    glEnableVertexArrayAttrib(m_impl->vao, 1);
    glEnableVertexArrayAttrib(m_impl->vao, 2);
    glVertexArrayAttribBinding(m_impl->vao, 0, 0);
    glVertexArrayAttribBinding(m_impl->vao, 1, 0);
    glVertexArrayAttribBinding(m_impl->vao, 2, 0);

    // Индексный буфер
    glVertexArrayElementBuffer(m_impl->vao, m_impl->ebo);

    // Шейдер
    auto vs = std::make_unique<Shader>(ShaderType::Vertex, kVertexShader);
    auto fs = std::make_unique<Shader>(ShaderType::Fragment, kFragmentShader);
    m_impl->shader = std::make_unique<ShaderProgram>(*vs, *fs);

    // Белая текстура-заглушка
    m_impl->initWhiteTexture();

    // Предварительно аллоцируем память
    m_impl->sprites.reserve(Impl::kMaxSprites);
}

SpriteBatch::~SpriteBatch()
{
    if (m_impl->vao)
        glDeleteVertexArrays(1, &m_impl->vao);
    if (m_impl->vbo)
        glDeleteBuffers(1, &m_impl->vbo);
    if (m_impl->ebo)
        glDeleteBuffers(1, &m_impl->ebo);
    if (m_impl->whiteTexture)
        glDeleteTextures(1, &m_impl->whiteTexture);
}

void SpriteBatch::begin(const Camera& camera)
{
    m_impl->currentCamera = camera;
    m_impl->sprites.clear();
}

void SpriteBatch::draw(const SpriteDesc& desc)
{
    if (m_impl->sprites.size() >= Impl::kMaxSprites)
        return;
    m_impl->sprites.push_back(desc);
}

void SpriteBatch::end()
{
    if (m_impl->sprites.empty())
        return;

    // Группируем спрайты по текстурам
    std::unordered_map<uint32_t, std::vector<const SpriteDesc*>> groups;
    for (const auto& sprite : m_impl->sprites)
    {
        uint32_t texId = sprite.textureId ? sprite.textureId : m_impl->whiteTexture;
        groups[texId].push_back(&sprite);
    }

    m_impl->shader->bind();
    auto proj = m_impl->currentCamera.matrix();
    m_impl->shader->setUniformMat4("uProjection", proj.data());

    glBindVertexArray(m_impl->vao);

    // Рендерим каждую группу отдельно
    for (const auto& [texId, spritePtrs] : groups)
    {
        std::vector<Impl::Vertex> vertices;
        vertices.reserve(spritePtrs.size() * Impl::kVerticesPerSprite);

        for (const auto* desc : spritePtrs)
        {
            float cosR = std::cos(desc->rot);
            float sinR = std::sin(desc->rot);
            float hw = desc->sx * 0.5f;
            float hh = desc->sy * 0.5f;

            float corners[4][2] = {
                { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh }
            };

            float texCoords[4][2] = {
                { desc->u0, desc->v1 }, { desc->u1, desc->v1 }, { desc->u1, desc->v0 }, { desc->u0, desc->v0 }
            };

            for (int i = 0; i < 4; ++i)
            {
                float lx = corners[i][0];
                float ly = corners[i][1];

                Impl::Vertex v;
                v.x = desc->x + lx * cosR - ly * sinR;
                v.y = desc->y + lx * sinR + ly * cosR;
                v.u = texCoords[i][0];
                v.v = texCoords[i][1];
                v.r = desc->r;
                v.g = desc->g;
                v.b = desc->b;
                v.a = desc->a;

                vertices.push_back(v);
            }
        }

        // Индексы
        std::vector<uint32_t> indices;
        size_t spriteCount = spritePtrs.size();
        indices.reserve(spriteCount * Impl::kIndicesPerSprite);
        for (size_t i = 0; i < spriteCount; ++i)
        {
            uint32_t offset = static_cast<uint32_t>(i * Impl::kVerticesPerSprite);
            indices.insert(indices.end(), { offset, offset + 1, offset + 2, offset, offset + 2, offset + 3 });
        }

        // Загружаем на GPU
        glNamedBufferData(m_impl->vbo, vertices.size() * sizeof(Impl::Vertex), vertices.data(), GL_DYNAMIC_DRAW);

        glNamedBufferData(m_impl->ebo, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);

        // Привязываем текстуру группы
        glBindTextureUnit(0, texId);

        // Рисуем
        glDrawElements(GL_TRIANGLES, static_cast<int>(indices.size()), GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    ShaderProgram::unbind();
}

} // namespace wingz::gfx
