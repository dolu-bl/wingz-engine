#pragma once

#include <array>
#include <cstdint>
#include <memory>

struct GLFWwindow;

namespace wingz::input
{

/// Коды клавиш (обёртка над GLFW для независимости).
enum class Key : uint16_t
{
    Unknown = 0,
    Space,
    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    D0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    D9,
    Semicolon,
    Equal,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LeftBracket,
    Backslash,
    RightBracket,
    GraveAccent,
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,

    Count
};

/// Коды кнопок мыши.
enum class MouseButton : uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7,

    Count
};

/// Состояние клавиши/кнопки в текущем кадре.
enum class InputState : uint8_t
{
    Released,
    Pressed,
    Held,
};

/// Сырые данные ввода за кадр.
struct InputSnapshot
{
    std::array<InputState, static_cast<size_t>(Key::Count)> keys {};
    std::array<InputState, static_cast<size_t>(MouseButton::Count)> mouseButtons {};
    double mouseX = 0.0;
    double mouseY = 0.0;
    double mouseScrollX = 0.0;
    double mouseScrollY = 0.0;
};

/// Менеджер ввода. Обрабатывает колбэки GLFW и предоставляет снапшот.
class InputManager
{
public:
    InputManager();
    ~InputManager();

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /// Привязывает колбэки к окну GLFW.
    void attach(GLFWwindow* window);

    /// Вызывается в начале кадра. Сохраняет предыдущее состояние.
    void beginFrame();

    /// Вызывается в конце кадра. Сбрасывает Pressed → Held.
    void endFrame();

    /// Возвращает снапшот за текущий кадр (только для чтения).
    const InputSnapshot& snapshot() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::input
