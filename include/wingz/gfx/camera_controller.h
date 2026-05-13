#pragma once

#include <memory>

#include <wingz/gfx/camera.h>

namespace wingz::gfx
{

/// Контроллер камеры.
/// Не знает о клавишах — управляется через методы извне.
class CameraController
{
public:
    CameraController();
    ~CameraController();

    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    /// Обновить камеру. Вызывать каждый кадр.
    /// @param dt Время с последнего кадра.
    /// @param targetX Позиция цели по X (для режима следования).
    /// @param targetY Позиция цели по Y (для режима следования).
    void update(float dt, float targetX, float targetY);

    /// Текущая камера.
    const Camera& camera() const;

    /// Переключить режим (следование / свободный).
    void toggleMode();

    /// Текущий режим: true = следование, false = свободный.
    bool isFollowing() const;

    /// Установить границы мира.
    void setWorldBounds(float left, float right, float top, float bottom);

    /// Двигать камеру влево с указанной скоростью (0..1).
    void moveLeft(float intensity = 1.0f);

    /// Двигать камеру вправо.
    void moveRight(float intensity = 1.0f);

    /// Двигать камеру вверх.
    void moveUp(float intensity = 1.0f);

    /// Двигать камеру вниз.
    void moveDown(float intensity = 1.0f);

    /// Установить скорость свободной камеры (пикселей/сек).
    void setFreeSpeed(float speed);

    /// Мгновенно телепортировать камеру.
    void setPosition(float x, float y);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
