#include <array>
#include <stdexcept>

#include <glad/glad.h>

#include <spdlog/spdlog.h>

#include "wingz/gfx/shader.h"

namespace wingz::gfx
{

// -----------------------------------------------------------------------------
// Shader
// -----------------------------------------------------------------------------

Shader::Shader(ShaderType type, std::string_view source)
    : m_handle(0)
{
    GLenum glType = 0;
    switch (type)
    {
    case ShaderType::Vertex:
        glType = GL_VERTEX_SHADER;
        break;
    case ShaderType::Fragment:
        glType = GL_FRAGMENT_SHADER;
        break;
    case ShaderType::Geometry:
        glType = GL_GEOMETRY_SHADER;
        break;
    case ShaderType::Compute:
        glType = GL_COMPUTE_SHADER;
        break;
    }

    m_handle = glCreateShader(glType);
    if (!m_handle)
        throw std::runtime_error("Не удалось создать шейдер");

    const char* src = source.data();
    int length = static_cast<int>(source.length());
    glShaderSource(m_handle, 1, &src, &length);
    glCompileShader(m_handle);

    // Проверяем ошибки компиляции
    int success = 0;
    glGetShaderiv(m_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        std::array<char, 512> log {};
        glGetShaderInfoLog(m_handle, log.size(), nullptr, log.data());
        glDeleteShader(m_handle);
        throw std::runtime_error(std::string("Ошибка компиляции шейдера: ") + log.data());
    }
}

Shader::~Shader()
{
    if (m_handle)
        glDeleteShader(m_handle);
}

Shader::Shader(Shader&& other) noexcept
    : m_handle(other.m_handle)
{
    other.m_handle = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (m_handle)
            glDeleteShader(m_handle);
        m_handle = other.m_handle;
        other.m_handle = 0;
    }
    return *this;
}

uint32_t Shader::handle() const
{
    return m_handle;
}

// -----------------------------------------------------------------------------
// ShaderProgram
// -----------------------------------------------------------------------------

ShaderProgram::ShaderProgram(const Shader& vertex, const Shader& fragment)
    : m_handle(0)
{
    m_handle = glCreateProgram();
    if (!m_handle)
        throw std::runtime_error("Не удалось создать шейдерную программу");

    glAttachShader(m_handle, vertex.handle());
    glAttachShader(m_handle, fragment.handle());
    glLinkProgram(m_handle);

    int success = 0;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::array<char, 512> log {};
        glGetProgramInfoLog(m_handle, log.size(), nullptr, log.data());
        glDeleteProgram(m_handle);
        throw std::runtime_error(std::string("Ошибка линковки шейдерной программы: ") + log.data());
    }

    // Шейдеры больше не нужны, программа слинкована
    glDetachShader(m_handle, vertex.handle());
    glDetachShader(m_handle, fragment.handle());
}

ShaderProgram::~ShaderProgram()
{
    if (m_handle)
        glDeleteProgram(m_handle);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_handle(other.m_handle)
{
    other.m_handle = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        if (m_handle)
            glDeleteProgram(m_handle);
        m_handle = other.m_handle;
        other.m_handle = 0;
    }
    return *this;
}

void ShaderProgram::bind() const
{
    glUseProgram(m_handle);
}

void ShaderProgram::unbind()
{
    glUseProgram(0);
}

uint32_t ShaderProgram::handle() const
{
    return m_handle;
}

void ShaderProgram::setUniformMat4(std::string_view name, const float* data) const
{
    int location = glGetUniformLocation(m_handle, name.data());
    if (location != -1)
        glUniformMatrix4fv(location, 1, GL_FALSE, data);
}

} // namespace wingz::gfx
