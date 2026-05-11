#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace wingz::gfx
{

/// Типы шейдеров.
enum class ShaderType : uint32_t
{
    Vertex,
    Fragment,
    Geometry,
    Compute
};

/// Скомпилированный шейдер (vertex, fragment или другой).
/// После компиляции используется в ShaderProgram.
class Shader
{
public:
    Shader(ShaderType type, std::string_view source);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    uint32_t handle() const;

private:
    uint32_t m_handle;
};

/// Слинкованная шейдерная программа.
class ShaderProgram
{
public:
    /// Создаёт программу из вершинного и фрагментного шейдера.
    ShaderProgram(const Shader& vertex, const Shader& fragment);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /// Активирует программу для рендеринга.
    void bind() const;

    /// Деактивирует программу.
    static void unbind();

    uint32_t handle() const;

    /// Устанавливает uniform-переменную.
    void setUniformMat4(std::string_view name, const float* data) const;

private:
    uint32_t m_handle;
};

} // namespace wingz::gfx
