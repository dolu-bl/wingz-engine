#pragma once

#include <memory>

struct GLFWwindow;

namespace wingz::gfx
{

/// Интеграция ImGui для отладочного UI.
class DebugUI
{
public:
    DebugUI(GLFWwindow* window);
    ~DebugUI();

    DebugUI(const DebugUI&) = delete;
    DebugUI& operator=(const DebugUI&) = delete;

    /// Начинает новый кадр ImGui.
    void beginFrame();

    /// Завершает кадр и вызывает рендеринг.
    void endFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
