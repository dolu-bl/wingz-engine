#pragma once

#include <functional>
#include <memory>
#include <string>

struct GLFWwindow;

namespace wingz
{

/// Дескриптор окна для создания.
struct WindowDesc
{
    int width { 1280 };
    int height { 720 };
    std::string title { "Wingz Engine" };
    bool fullscreen { false };
    bool resizable { true };
};

/// Событие изменения размера окна.
struct WindowResizeEvent
{
    int width;
    int height;
};

/// Оборачивает GLFW-окно, управляет жизненным циклом.
class Window
{
public:
    explicit Window(const WindowDesc& desc);
    ~Window();

    // Запрещаем копирование
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// Должно ли окно закрыться в этом кадре.
    bool shouldClose() const;

    /// Обрабатывает события окна (клавиатура, мышь, ресайз и т.д.).
    void pollEvents();

    /// Меняет буферы местами.
    void swapBuffers();

    /// Нативный хендл GLFW (нужен для ImGui).
    GLFWwindow* nativeHandle() const;

    /// Текущие размеры окна.
    int width() const;
    int height() const;

    /// Колбэк на изменение размера (вызывается из GLFW-колбэка).
    using ResizeCallback = std::function<void(const WindowResizeEvent&)>;
    void setResizeCallback(ResizeCallback cb);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz
