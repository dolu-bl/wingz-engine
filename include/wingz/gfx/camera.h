#pragma once

#include <array>

namespace wingz::gfx
{

/// Ортогональная 2D-камера.
struct Camera
{
    float left = 0.0f;
    float right = 1280.0f;
    float bottom = 720.0f; // инвертировано для экранных координат
    float top = 0.0f;

    /// Возвращает матрицу проекции 4x4 (column-major, совместимо с OpenGL).
    std::array<float, 16> matrix() const;
};

} // namespace wingz::gfx
