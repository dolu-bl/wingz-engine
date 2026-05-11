#include <chrono>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "app.h"
#include "window.h"

namespace wingz
{

struct App::Impl
{
    std::unique_ptr<Window> window;
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
        throw std::runtime_error("Окно ещё не создано. Вызовите createWindow() в onInit()");
    return *m_impl->window;
}

int App::run(int argc, char** argv)
{
    try
    {
        onInit();

        if (!m_impl->window)
            throw std::runtime_error("onInit() не создал окно. Вызовите createWindow() в onInit()");

        spdlog::info("Приложение запущено");

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
            m_impl->window->swapBuffers();
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
