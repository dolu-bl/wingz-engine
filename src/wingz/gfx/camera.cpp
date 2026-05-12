#include <algorithm>
#include <cmath>

#include "wingz/gfx/camera.h"

namespace wingz::gfx
{

std::array<float, 16> Camera::matrix() const
{
    // Ортографическая проекция, column-major
    float w = right - left;
    float h = top - bottom;

    std::array<float, 16> m {};
    m[0] = 2.0f / w; // [0,0]
    m[5] = 2.0f / h; // [1,1]
    m[10] = -1.0f; // [2,2]
    m[12] = -(right + left) / w; // [3,0] — translation X
    m[13] = -(top + bottom) / h; // [3,1] — translation Y
    m[15] = 1.0f; // [3,3]

    return m;
}

} // namespace wingz::gfx
