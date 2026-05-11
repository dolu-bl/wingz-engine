#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <wingz/app.h>
#include <wingz/gfx/camera.h>
#include <wingz/gfx/sprite_batch.h>
#include <wingz/window.h>

/// Тестовое приложение с первым спрайтом.
class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        createWindow(wingz::WindowDesc { .width = 1280, .height = 720, .title = "Wingz Engine — Sandbox" });

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        spdlog::info("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
        spdlog::info("GPU: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        window().setResizeCallback([](const wingz::WindowResizeEvent& e)
                                   { glViewport(0, 0, e.width, e.height); });

        glViewport(0, 0, window().width(), window().height());

        // Включаем блендинг
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Создаём батчер
        m_spriteBatch = std::make_unique<wingz::gfx::SpriteBatch>();

        // Настраиваем камеру (экранные координаты)
        m_camera.left = 0.0f;
        m_camera.right = static_cast<float>(window().width());
        m_camera.bottom = static_cast<float>(window().height());
        m_camera.top = 0.0f;
    }

    void onUpdate(float dt) override
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Рисуем тестовый квадрат
        m_spriteBatch->begin(m_camera);

        wingz::gfx::SpriteDesc desc;
        desc.x = 640.0f;
        desc.y = 360.0f;
        desc.sx = 200.0f;
        desc.sy = 200.0f;
        desc.rot = static_cast<float>(glfwGetTime()); // вращается!
        desc.r = 0.2f;
        desc.g = 0.6f;
        desc.b = 1.0f;
        desc.a = 1.0f;

        m_spriteBatch->draw(desc);
        m_spriteBatch->end();
    }

    void onShutdown() override
    {
        spdlog::info("Пока!");
    }

    std::unique_ptr<wingz::gfx::SpriteBatch> m_spriteBatch;
    wingz::gfx::Camera m_camera;
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}
