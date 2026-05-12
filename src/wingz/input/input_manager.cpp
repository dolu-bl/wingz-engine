#include <algorithm>
#include <cstring>

#include <GLFW/glfw3.h>

#include "wingz/input/input_manager.h"

namespace wingz::input
{

static Key mapGlfwKey(int glfwKey)
{
    switch (glfwKey)
    {
    case GLFW_KEY_SPACE:
        return Key::Space;
    case GLFW_KEY_APOSTROPHE:
        return Key::Apostrophe;
    case GLFW_KEY_COMMA:
        return Key::Comma;
    case GLFW_KEY_MINUS:
        return Key::Minus;
    case GLFW_KEY_PERIOD:
        return Key::Period;
    case GLFW_KEY_SLASH:
        return Key::Slash;
    case GLFW_KEY_0:
        return Key::D0;
    case GLFW_KEY_1:
        return Key::D1;
    case GLFW_KEY_2:
        return Key::D2;
    case GLFW_KEY_3:
        return Key::D3;
    case GLFW_KEY_4:
        return Key::D4;
    case GLFW_KEY_5:
        return Key::D5;
    case GLFW_KEY_6:
        return Key::D6;
    case GLFW_KEY_7:
        return Key::D7;
    case GLFW_KEY_8:
        return Key::D8;
    case GLFW_KEY_9:
        return Key::D9;
    case GLFW_KEY_SEMICOLON:
        return Key::Semicolon;
    case GLFW_KEY_EQUAL:
        return Key::Equal;
    case GLFW_KEY_A:
        return Key::A;
    case GLFW_KEY_B:
        return Key::B;
    case GLFW_KEY_C:
        return Key::C;
    case GLFW_KEY_D:
        return Key::D;
    case GLFW_KEY_E:
        return Key::E;
    case GLFW_KEY_F:
        return Key::F;
    case GLFW_KEY_G:
        return Key::G;
    case GLFW_KEY_H:
        return Key::H;
    case GLFW_KEY_I:
        return Key::I;
    case GLFW_KEY_J:
        return Key::J;
    case GLFW_KEY_K:
        return Key::K;
    case GLFW_KEY_L:
        return Key::L;
    case GLFW_KEY_M:
        return Key::M;
    case GLFW_KEY_N:
        return Key::N;
    case GLFW_KEY_O:
        return Key::O;
    case GLFW_KEY_P:
        return Key::P;
    case GLFW_KEY_Q:
        return Key::Q;
    case GLFW_KEY_R:
        return Key::R;
    case GLFW_KEY_S:
        return Key::S;
    case GLFW_KEY_T:
        return Key::T;
    case GLFW_KEY_U:
        return Key::U;
    case GLFW_KEY_V:
        return Key::V;
    case GLFW_KEY_W:
        return Key::W;
    case GLFW_KEY_X:
        return Key::X;
    case GLFW_KEY_Y:
        return Key::Y;
    case GLFW_KEY_Z:
        return Key::Z;
    case GLFW_KEY_LEFT_BRACKET:
        return Key::LeftBracket;
    case GLFW_KEY_BACKSLASH:
        return Key::Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
        return Key::RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
        return Key::GraveAccent;
    case GLFW_KEY_ESCAPE:
        return Key::Escape;
    case GLFW_KEY_ENTER:
        return Key::Enter;
    case GLFW_KEY_TAB:
        return Key::Tab;
    case GLFW_KEY_BACKSPACE:
        return Key::Backspace;
    case GLFW_KEY_INSERT:
        return Key::Insert;
    case GLFW_KEY_DELETE:
        return Key::Delete;
    case GLFW_KEY_RIGHT:
        return Key::Right;
    case GLFW_KEY_LEFT:
        return Key::Left;
    case GLFW_KEY_DOWN:
        return Key::Down;
    case GLFW_KEY_UP:
        return Key::Up;
    case GLFW_KEY_PAGE_UP:
        return Key::PageUp;
    case GLFW_KEY_PAGE_DOWN:
        return Key::PageDown;
    case GLFW_KEY_HOME:
        return Key::Home;
    case GLFW_KEY_END:
        return Key::End;
    case GLFW_KEY_CAPS_LOCK:
        return Key::CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
        return Key::ScrollLock;
    case GLFW_KEY_NUM_LOCK:
        return Key::NumLock;
    case GLFW_KEY_PRINT_SCREEN:
        return Key::PrintScreen;
    case GLFW_KEY_PAUSE:
        return Key::Pause;
    case GLFW_KEY_F1:
        return Key::F1;
    case GLFW_KEY_F2:
        return Key::F2;
    case GLFW_KEY_F3:
        return Key::F3;
    case GLFW_KEY_F4:
        return Key::F4;
    case GLFW_KEY_F5:
        return Key::F5;
    case GLFW_KEY_F6:
        return Key::F6;
    case GLFW_KEY_F7:
        return Key::F7;
    case GLFW_KEY_F8:
        return Key::F8;
    case GLFW_KEY_F9:
        return Key::F9;
    case GLFW_KEY_F10:
        return Key::F10;
    case GLFW_KEY_F11:
        return Key::F11;
    case GLFW_KEY_F12:
        return Key::F12;
    case GLFW_KEY_LEFT_SHIFT:
        return Key::LeftShift;
    case GLFW_KEY_LEFT_CONTROL:
        return Key::LeftControl;
    case GLFW_KEY_LEFT_ALT:
        return Key::LeftAlt;
    case GLFW_KEY_LEFT_SUPER:
        return Key::LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
        return Key::RightShift;
    case GLFW_KEY_RIGHT_CONTROL:
        return Key::RightControl;
    case GLFW_KEY_RIGHT_ALT:
        return Key::RightAlt;
    case GLFW_KEY_RIGHT_SUPER:
        return Key::RightSuper;
    case GLFW_KEY_MENU:
        return Key::Menu;
    default:
        return Key::Unknown;
    }
}

static MouseButton mapGlfwMouse(int button)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:
        return MouseButton::Left;
    case GLFW_MOUSE_BUTTON_2:
        return MouseButton::Right;
    case GLFW_MOUSE_BUTTON_3:
        return MouseButton::Middle;
    case GLFW_MOUSE_BUTTON_4:
        return MouseButton::Button4;
    case GLFW_MOUSE_BUTTON_5:
        return MouseButton::Button5;
    case GLFW_MOUSE_BUTTON_6:
        return MouseButton::Button6;
    case GLFW_MOUSE_BUTTON_7:
        return MouseButton::Button7;
    case GLFW_MOUSE_BUTTON_8:
        return MouseButton::Button8;
    default:
        return MouseButton::Left;
    }
}

struct InputManager::Impl
{
    GLFWwindow* window = nullptr;
    InputSnapshot snapshot;
    InputSnapshot prevSnapshot;
};

InputManager::InputManager()
    : m_impl(std::make_unique<Impl>())
{
}

InputManager::~InputManager() = default;

void InputManager::attach(GLFWwindow* window)
{
    m_impl->window = window;
    // Колбэки не устанавливаем — опрашиваем GLFW напрямую в beginFrame()
}

void InputManager::beginFrame()
{
    // Копируем предыдущее состояние
    m_impl->prevSnapshot = m_impl->snapshot;

    if (!m_impl->window)
        return;

    // Опрашиваем клавиатуру
    for (int glfwKey = 0; glfwKey <= GLFW_KEY_LAST; ++glfwKey)
    {
        Key wk = mapGlfwKey(glfwKey);
        if (wk == Key::Unknown)
            continue;

        auto idx = static_cast<size_t>(wk);
        int state = glfwGetKey(m_impl->window, glfwKey);

        if (state == GLFW_PRESS)
        {
            if (m_impl->prevSnapshot.keys[idx] == InputState::Released)
                m_impl->snapshot.keys[idx] = InputState::Pressed;
            else
                m_impl->snapshot.keys[idx] = InputState::Held;
        }
        else
        {
            m_impl->snapshot.keys[idx] = InputState::Released;
        }
    }

    // Опрашиваем кнопки мыши
    for (int btn = GLFW_MOUSE_BUTTON_1; btn <= GLFW_MOUSE_BUTTON_LAST; ++btn)
    {
        MouseButton mb = mapGlfwMouse(btn);
        auto idx = static_cast<size_t>(mb);
        int state = glfwGetMouseButton(m_impl->window, btn);

        if (state == GLFW_PRESS)
        {
            if (m_impl->prevSnapshot.mouseButtons[idx] == InputState::Released)
                m_impl->snapshot.mouseButtons[idx] = InputState::Pressed;
            else
                m_impl->snapshot.mouseButtons[idx] = InputState::Held;
        }
        else
        {
            m_impl->snapshot.mouseButtons[idx] = InputState::Released;
        }
    }

    // Позиция мыши
    double mx, my;
    glfwGetCursorPos(m_impl->window, &mx, &my);
    m_impl->snapshot.mouseX = mx;
    m_impl->snapshot.mouseY = my;

    // Скролл сбрасываем (GLFW не даёт состояния скролла, только колбэк)
    // TODO: добавить колбэк только для скролла
    m_impl->snapshot.mouseScrollX = 0.0;
    m_impl->snapshot.mouseScrollY = 0.0;
}

void InputManager::endFrame()
{
    // Ничего не делаем — всё обрабатывается в beginFrame()
}

const InputSnapshot& InputManager::snapshot() const
{
    return m_impl->snapshot;
}

} // namespace wingz::input
