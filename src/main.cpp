#include <memory>
#include <stdexcept>
#include <string>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <spdlog/spdlog.h>

#include <wingz/app.h>
#include <wingz/ecs/components.h>
#include <wingz/gfx/camera.h>
#include <wingz/gfx/debug_ui.h>
#include <wingz/gfx/sprite_batch.h>
#include <wingz/gfx/texture.h>
#include <wingz/input/action_map.h>
#include <wingz/input/input_manager.h>
#include <wingz/scene.h>
#include <wingz/window.h>

class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        createWindow(wingz::WindowDesc { .width = 1280, .height = 720, .title = "Wingz Engine — ECS + Input" });

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
        m_debugUI = std::make_unique<wingz::gfx::DebugUI>(window().nativeHandle());

        // Ввод
        m_inputManager = std::make_unique<wingz::input::InputManager>();
        m_inputManager->attach(window().nativeHandle());
        m_actionMap = std::make_unique<wingz::input::ActionMap>();

        // ESC = выход
        m_actionMap->addAction(
            "move_up",
            wingz::input::InputBinding {
                .keys = { wingz::input::Key::W },
                .mouseButtons = {} },
            []()
            { spdlog::debug("W нажата"); }
        );

        // Сцена
        m_scene = std::make_unique<wingz::Scene>();
        m_scene->init();

        // Загружаем текстуру
        try
        {
            m_texture = std::make_unique<wingz::gfx::Texture>(
                "/home/mikhail/develop/wingz-engine/assets/test.png"
            );
            m_useTexture = true;
        }
        catch (const std::exception& e)
        {
            spdlog::warn("Тестовая текстура не загружена: {}", e.what());
            m_useTexture = false;
        }

        // Создаём тестовую сущность через ECS
        auto entity = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(entity, 640.0f, 360.0f, 0.0f);
        m_scene->registry().emplace<wingz::ecs::Velocity>(entity, 100.0f, 0.0f, 1.5f); // движется вправо, вращается
        m_scene->registry().emplace<wingz::ecs::Sprite>(entity, m_texture ? m_texture->handle() : 0u, 0.0f, 0.0f, 1.0f, 1.0f, 128.0f, 128.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        m_scene->registry().emplace<wingz::ecs::Tag>(entity, "Player");
        m_scene->registry().emplace<wingz::ecs::Player>(entity, 1);
    }

    void onUpdate(float dt) override
    {
        // 1. Обновляем состояния клавиш
        m_inputManager->beginFrame();

        // 2. Проверяем ESC напрямую (ImGui может перехватывать клавиатуру)
        auto& snap = m_inputManager->snapshot();
        if (snap.keys[static_cast<size_t>(wingz::input::Key::Escape)] == wingz::input::InputState::Pressed)
        {
            spdlog::info("ESC нажат, выход...");
            glfwSetWindowShouldClose(window().nativeHandle(), GLFW_TRUE);
            return;
        }

        // 3. Обрабатываем остальные действия
        m_actionMap->update(snap);

        // 4. Логика
        m_scene->update(dt);

        // 5. Рендер
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_scene->render(*m_spriteBatch);

        // 6. ImGui
        m_debugUI->beginFrame();
        ImGui::Begin("ECS Info");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Entities: %zu", m_scene->registry().storage<entt::entity>().in_use());
        ImGui::End();
        m_debugUI->endFrame();

        // 7. Завершаем кадр ввода
        m_inputManager->endFrame();
    }

    void onShutdown() override
    {
        spdlog::info("Пока!");
    }

    std::unique_ptr<wingz::gfx::SpriteBatch> m_spriteBatch;
    std::unique_ptr<wingz::gfx::DebugUI> m_debugUI;
    std::unique_ptr<wingz::input::InputManager> m_inputManager;
    std::unique_ptr<wingz::input::ActionMap> m_actionMap;
    std::unique_ptr<wingz::Scene> m_scene;
    std::unique_ptr<wingz::gfx::Texture> m_texture;
    bool m_useTexture = false;
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}
