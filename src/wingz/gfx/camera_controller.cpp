#include <algorithm>

#include <spdlog/spdlog.h>


#include <wingz/gfx/camera_controller.h>

namespace wingz::gfx
{

struct CameraController::Impl
{
    Camera cam;
    bool followMode = true;

    // Свободная камера
    float freeX = 640.0f;
    float freeY = 360.0f;
    float freeSpeed = 500.0f;

    // Текущее направление движения (суммируется из вызовов move*)
    float inputX = 0.0f;
    float inputY = 0.0f;

    // Границы мира
    float worldLeft = 0.0f;
    float worldRight = 1280.0f;
    float worldTop = 0.0f;
    float worldBottom = 720.0f;

    // Сглаживание следования
    float smoothX = 640.0f;
    float smoothY = 360.0f;
    float smoothFactor = 8.0f;
};

CameraController::CameraController()
    : m_impl(std::make_unique<Impl>())
{
    m_impl->cam.left = 0.0f;
    m_impl->cam.right = 1280.0f;
    m_impl->cam.bottom = 720.0f;
    m_impl->cam.top = 0.0f;
}

CameraController::~CameraController() = default;

void CameraController::update(float dt, float targetX, float targetY)
{
    if (m_impl->followMode)
    {
        m_impl->smoothX += (targetX - m_impl->smoothX) * m_impl->smoothFactor * dt;
        m_impl->smoothY += (targetY - m_impl->smoothY) * m_impl->smoothFactor * dt;
    }
    else
    {
        // Нормализуем диагональное движение
        float mx = m_impl->inputX;
        float my = m_impl->inputY;

        if (mx != 0.0f && my != 0.0f)
        {
            mx *= 0.707f;
            my *= 0.707f;
        }

        m_impl->freeX += mx * m_impl->freeSpeed * dt;
        m_impl->freeY += my * m_impl->freeSpeed * dt;

        // Ограничиваем границами мира
        float halfW = (m_impl->cam.right - m_impl->cam.left) * 0.5f;
        float halfH = (m_impl->cam.bottom - m_impl->cam.top) * 0.5f;

        m_impl->freeX = std::clamp(m_impl->freeX, m_impl->worldLeft + halfW, m_impl->worldRight - halfW);
        m_impl->freeY = std::clamp(m_impl->freeY, m_impl->worldTop + halfH, m_impl->worldBottom - halfH);

        m_impl->smoothX = m_impl->freeX;
        m_impl->smoothY = m_impl->freeY;

        // Сбрасываем ввод на следующий кадр
        m_impl->inputX = 0.0f;
        m_impl->inputY = 0.0f;
    }

    // Обновляем камеру
    float halfW = (m_impl->cam.right - m_impl->cam.left) * 0.5f;
    float halfH = (m_impl->cam.bottom - m_impl->cam.top) * 0.5f;

    m_impl->cam.left = m_impl->smoothX - halfW;
    m_impl->cam.right = m_impl->smoothX + halfW;
    m_impl->cam.top = m_impl->smoothY - halfH;
    m_impl->cam.bottom = m_impl->smoothY + halfH;
}

const Camera& CameraController::camera() const
{
    return m_impl->cam;
}

void CameraController::toggleMode()
{
    m_impl->followMode = !m_impl->followMode;
}

bool CameraController::isFollowing() const
{
    return m_impl->followMode;
}

void CameraController::setWorldBounds(float left, float right, float top, float bottom)
{
    m_impl->worldLeft = left;
    m_impl->worldRight = right;
    m_impl->worldTop = top;
    m_impl->worldBottom = bottom;
}

void CameraController::moveLeft(float intensity)
{
    m_impl->inputX -= intensity;
}

void CameraController::moveRight(float intensity)
{
    m_impl->inputX += intensity;
}

void CameraController::moveUp(float intensity)
{
    m_impl->inputY -= intensity;
}

void CameraController::moveDown(float intensity)
{
    m_impl->inputY += intensity;
}

void CameraController::setFreeSpeed(float speed)
{
    m_impl->freeSpeed = speed;
}

void CameraController::setPosition(float x, float y)
{
    m_impl->freeX = x;
    m_impl->freeY = y;
    m_impl->smoothX = x;
    m_impl->smoothY = y;
}

} // namespace wingz::gfx
