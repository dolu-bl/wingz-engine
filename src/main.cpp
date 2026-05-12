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
#include <wingz/physics/collider.h>
#include <wingz/scene.h>
#include <wingz/window.h>

class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        createWindow(
            wingz::WindowDesc {
                .width = 1280,
                .height = 720,
                .title = "Wingz Engine — Physics" }
        );

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        spdlog::info("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
        spdlog::info("GPU: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        window().setResizeCallback(
            [](const wingz::WindowResizeEvent& e)
            {
                glViewport(0, 0, e.width, e.height);
            }
        );

        glViewport(0, 0, window().width(), window().height());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Графика
        m_spriteBatch = std::make_unique<wingz::gfx::SpriteBatch>();
        m_debugUI = std::make_unique<wingz::gfx::DebugUI>(
            window().nativeHandle()
        );

        // Ввод
        m_inputManager = std::make_unique<wingz::input::InputManager>();
        m_inputManager->attach(window().nativeHandle());
        m_actionMap = std::make_unique<wingz::input::ActionMap>();

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

        uint32_t texId = m_texture ? m_texture->handle() : 0u;

        // Игрок (управляется WASD)
        m_playerEntity = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(
            m_playerEntity, 400.0f, 360.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Velocity>(
            m_playerEntity, 0.0f, 0.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Sprite>(
            m_playerEntity, texId, 0.0f, 0.0f, 1.0f, 1.0f, 64.0f, 64.0f, 0.2f, 0.6f, 1.0f, 1.0f
        );
        m_scene->registry().emplace<wingz::ecs::Tag>(
            m_playerEntity, "Player"
        );
        m_scene->registry().emplace<wingz::ecs::Player>(
            m_playerEntity, 1
        );
        m_scene->registry().emplace<wingz::ecs::InputIntent>(
            m_playerEntity, 0.0f, 0.0f, false
        );
        m_scene->registry().emplace<wingz::physics::Collider>(
            m_playerEntity,
            wingz::physics::AABB { 0.0f, 0.0f, 32.0f, 32.0f },
            wingz::physics::CollisionLayer::Player,
            wingz::physics::CollisionLayer::Wall | wingz::physics::CollisionLayer::Enemy,
            false,
            false
        );

        // Стена (статическая)
        auto wall = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(
            wall, 640.0f, 360.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Sprite>(
            wall, texId, 0.0f, 0.0f, 1.0f, 1.0f, 128.0f, 32.0f, 0.8f, 0.2f, 0.2f, 1.0f
        );
        m_scene->registry().emplace<wingz::ecs::Tag>(
            wall, "Wall"
        );
        m_scene->registry().emplace<wingz::physics::Collider>(
            wall, wingz::physics::AABB { 0.0f, 0.0f, 64.0f, 16.0f },
            wingz::physics::CollisionLayer::Wall,
            wingz::physics::CollisionLayer::Player,
            false,
            true
        );

        // Враг (статический, для теста коллизий)
        auto enemy = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(
            enemy, 700.0f, 200.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Sprite>(
            enemy, texId, 0.0f, 0.0f, 1.0f, 1.0f, 48.0f, 48.0f, 1.0f, 0.3f, 0.3f, 1.0f
        );
        m_scene->registry().emplace<wingz::ecs::Tag>(
            enemy, "Enemy"
        );
        m_scene->registry().emplace<wingz::physics::Collider>(
            enemy, wingz::physics::AABB { 0.0f, 0.0f, 24.0f, 24.0f },
            wingz::physics::CollisionLayer::Enemy,
            wingz::physics::CollisionLayer::Player,
            false,
            true
        );
    }

    void onUpdate(float dt) override
    {
        // Ввод
        m_inputManager->beginFrame();

        auto& snap = m_inputManager->snapshot();
        if (wingz::input::InputState::Pressed == snap.keys[static_cast<size_t>(wingz::input::Key::Escape)])
        {
            spdlog::info("ESC нажат, выход...");
            glfwSetWindowShouldClose(window().nativeHandle(), GLFW_TRUE);
            return;
        }

        // Обработка WASD
        float moveX = 0.0f;
        float moveY = 0.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::A)] == wingz::input::InputState::Held)
            moveX -= 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::D)] == wingz::input::InputState::Held)
            moveX += 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::W)] == wingz::input::InputState::Held)
            moveY -= 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::S)] == wingz::input::InputState::Held)
            moveY += 1.0f;

        if (m_scene->registry().valid(m_playerEntity))
        {
            auto& intent = m_scene->registry().get<wingz::ecs::InputIntent>(m_playerEntity);
            intent.moveX = moveX;
            intent.moveY = moveY;
        }

        // Логика
        m_scene->update(dt);

        // Рендер
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_scene->render(*m_spriteBatch);

        // ImGui
        m_debugUI->beginFrame();
        ImGui::Begin("Physics Info");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Entities: %zu", m_scene->registry().storage<entt::entity>().in_use());

        if (m_scene->registry().valid(m_playerEntity))
        {
            auto& transform = m_scene->registry().get<wingz::ecs::Transform>(m_playerEntity);
            ImGui::Text("Player pos: (%.1f, %.1f)", transform.x, transform.y);
        }

        ImGui::End();
        m_debugUI->endFrame();

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
    entt::entity m_playerEntity = entt::null;
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}
