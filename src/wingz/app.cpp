#include <chrono>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "wingz/app.h"
#include "wingz/gfx/imgui_context.h"
#include "wingz/input/input_manager.h"
#include "wingz/window.h"

namespace wingz
{

struct App::Impl
{
    std::unique_ptr<Window> window;
    std::unique_ptr<input::InputManager> inputManager;
    std::unique_ptr<gfx::ImGuiContext> imGui;
    core::StateStack stateStack;
};

App::App()
    : m_impl(std::make_unique<Impl>())
{
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
}

App::~App() = default;

void App::createWindow(const WindowDesc& desc)
{
    m_impl->window = std::make_unique<Window>(desc);
}

Window& App::window()
{
    if (!m_impl->window)
        throw std::runtime_error("Окно не создано");
    return *m_impl->window;
}

void App::createInput()
{
    if (!m_impl->window)
        throw std::runtime_error("Сначала создайте окно");
    m_impl->inputManager = std::make_unique<input::InputManager>();
    m_impl->inputManager->attach(m_impl->window->nativeHandle());
}

void App::createImGui()
{
    if (!m_impl->window)
        throw std::runtime_error("Сначала создайте окно");
    m_impl->imGui = std::make_unique<gfx::ImGuiContext>(m_impl->window->nativeHandle());
}

core::StateStack& App::stateStack()
{
    return m_impl->stateStack;
}

core::StateContext App::createContext()
{
    core::StateContext ctx;
    ctx.stack = &m_impl->stateStack;
    ctx.window = m_impl->window.get();
    ctx.inputManager = m_impl->inputManager.get();
    ctx.imGui = m_impl->imGui.get();
    return ctx;
}

int App::run(int argc, char** argv)
{
    try
    {
        onInit();

        if (!m_impl->window)
            throw std::runtime_error("onInit() не создал окно");

        if (m_impl->stateStack.empty())
            spdlog::warn("Стек состояний пуст");

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!m_impl->window->shouldClose())
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            if (dt > 0.1f)
                dt = 0.1f;

            m_impl->window->pollEvents();

            onUpdate(dt);

            m_impl->stateStack.update(dt);
            m_impl->stateStack.render();
            m_impl->window->swapBuffers();

            if (m_impl->stateStack.empty())
            {
                spdlog::info("Стек состояний пуст, выход");
                break;
            }
        }

        onShutdown();
        spdlog::info("Приложение завершено");
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Критическая ошибка: {}", e.what());
        return EXIT_FAILURE;
    }
}

} // namespace wingz
