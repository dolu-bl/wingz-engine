#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <spdlog/spdlog.h>

#include <wingz/app.h>
#include <wingz/gfx/camera.h>
#include <wingz/gfx/debug_ui.h>
#include <wingz/gfx/sprite_batch.h>
#include <wingz/gfx/texture.h>
#include <wingz/window.h>

/// Тестовое приложение с ImGui и текстурой.
class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        createWindow(wingz::WindowDesc { .width = 1280, .height = 720, .title = "Wingz Engine — Sprite + ImGui" });

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        spdlog::info("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
        spdlog::info("GPU: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        window().setResizeCallback([](const wingz::WindowResizeEvent& e)
                                   { glViewport(0, 0, e.width, e.height); });

        glViewport(0, 0, window().width(), window().height());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Графика
        m_spriteBatch = std::make_unique<wingz::gfx::SpriteBatch>();

        m_camera.left = 0.0f;
        m_camera.right = static_cast<float>(window().width());
        m_camera.bottom = static_cast<float>(window().height());
        m_camera.top = 0.0f;

        // ImGui
        m_debugUI = std::make_unique<wingz::gfx::DebugUI>(window().nativeHandle());

        // Загружаем текстуру (замените путь на свою или создайте заглушку)
        try
        {
            m_texture = std::make_unique<wingz::gfx::Texture>(
                "/mnt/develop/github/wingz-engine/assets/test.png"
            );
            m_useTexture = true;
        }
        catch (const std::exception& e)
        {
            spdlog::warn("Тестовая текстура не загружена: {}", e.what());
            m_useTexture = false;
        }
    }

    void onUpdate(float dt) override
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        static float rotation = 0.0f;
        rotation += dt;

        // Рисуем спрайты
        m_spriteBatch->begin(m_camera);

        wingz::gfx::SpriteDesc desc;
        desc.x = 640.0f;
        desc.y = 360.0f;
        desc.sx = 256.0f;
        desc.sy = 256.0f;
        desc.rot = rotation;

        if (m_useTexture && m_texture)
        {
            desc.textureId = m_texture->handle();
            desc.u0 = 0.0f;
            desc.v0 = 0.0f;
            desc.u1 = 1.0f;
            desc.v1 = 1.0f;
            desc.r = 1.0f;
            desc.g = 1.0f;
            desc.b = 1.0f;
            desc.a = 1.0f;
        }
        else
        {
            desc.textureId = 0;
            desc.r = 0.2f;
            desc.g = 0.6f;
            desc.b = 1.0f;
            desc.a = 1.0f;
        }

        m_spriteBatch->draw(desc);
        m_spriteBatch->end();

        // ImGui поверх всего
        m_debugUI->beginFrame();

        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Frame time: %.3f ms", dt * 1000.0f);
        ImGui::Text("Rotation: %.2f", rotation);
        ImGui::Checkbox("Use texture", &m_useTexture);
        ImGui::End();

        m_debugUI->endFrame();
    }

    void onShutdown() override
    {
        spdlog::info("Пока!");
    }

    std::unique_ptr<wingz::gfx::SpriteBatch> m_spriteBatch;
    wingz::gfx::Camera m_camera;
    std::unique_ptr<wingz::gfx::DebugUI> m_debugUI;
    std::unique_ptr<wingz::gfx::Texture> m_texture;
    bool m_useTexture = false;
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}
