#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <wingz/app.h>
#include <wingz/window.h>

/// Тестовое приложение, показывающее чёрное окно.
class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        // Создаём окно через базовый класс
        createWindow(wingz::WindowDesc { .width = 1280, .height = 720, .title = "Wingz Engine — Sandbox" });

        // Загружаем OpenGL через glad
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        spdlog::info("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
        spdlog::info("GPU: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        // Настраиваем колбэк ресайза
        window().setResizeCallback([](const wingz::WindowResizeEvent& e)
                                   {
            glViewport(0, 0, e.width, e.height);
            spdlog::debug("Ресайз: {}x{}", e.width, e.height); });

        // Начальный viewport
        glViewport(0, 0, window().width(), window().height());
    }

    void onUpdate(float dt) override
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void onShutdown() override
    {
        spdlog::info("Пока!");
    }
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}