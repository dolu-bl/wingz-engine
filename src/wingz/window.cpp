#include <cassert>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "window.h"

namespace wingz
{

struct Window::Impl
{
    GLFWwindow* handle { nullptr };
    WindowDesc desc;
    ResizeCallback resizeCallback;
    bool isMinimized { false };

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* impl = static_cast<Impl*>(glfwGetWindowUserPointer(window));
        if (!impl)
            return;

        impl->isMinimized = (width == 0 || height == 0);
        impl->desc.width = width;
        impl->desc.height = height;

        if (impl->resizeCallback)
            impl->resizeCallback({ width, height });
    }
};

Window::Window(const WindowDesc& desc)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->desc = desc;

    if (!glfwInit())
        throw std::runtime_error("Не удалось инициализировать GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

    // Активируем отладку в Debug-сборке
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWmonitor* monitor = desc.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    m_impl->handle = glfwCreateWindow(
        desc.width,
        desc.height,
        desc.title.c_str(),
        monitor,
        nullptr
    );

    if (!m_impl->handle)
    {
        glfwTerminate();
        throw std::runtime_error("Не удалось создать GLFW-окно");
    }

    glfwSetWindowUserPointer(m_impl->handle, m_impl.get());
    glfwSetFramebufferSizeCallback(m_impl->handle, Impl::framebufferSizeCallback);

    glfwMakeContextCurrent(m_impl->handle);
    glfwSwapInterval(1); // V-Sync
}

Window::~Window()
{
    if (m_impl->handle)
        glfwDestroyWindow(m_impl->handle);
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_impl->handle);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::swapBuffers()
{
    glfwSwapBuffers(m_impl->handle);
}

GLFWwindow* Window::nativeHandle() const
{
    return m_impl->handle;
}

int Window::width() const
{
    return m_impl->desc.width;
}

int Window::height() const
{
    return m_impl->desc.height;
}

void Window::setResizeCallback(ResizeCallback cb)
{
    m_impl->resizeCallback = std::move(cb);
}

} // namespace wingz
