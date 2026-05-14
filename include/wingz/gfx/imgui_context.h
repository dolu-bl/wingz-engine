#pragma once

#include <memory>

struct GLFWwindow;

namespace wingz::gfx
{

/// Владеет контекстом ImGui для всего приложения.
/// Создаётся один раз. Состояния используют его через beginFrame/endFrame.
class ImGuiContext
{
public:
    explicit ImGuiContext(GLFWwindow* window);
    ~ImGuiContext();

    ImGuiContext(const ImGuiContext&) = delete;
    ImGuiContext& operator=(const ImGuiContext&) = delete;

    void beginFrame();
    void endFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz::gfx
