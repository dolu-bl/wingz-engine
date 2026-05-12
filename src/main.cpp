#include <cstring>
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
#include <wingz/net/host.h>
#include <wingz/net/replication.h>
#include <wingz/net/types.h>
#include <wingz/physics/collider.h>
#include <wingz/scene.h>
#include <wingz/window.h>

class SandboxApp : public wingz::App
{
public:
    explicit SandboxApp(bool isServer)
        : m_isServer(isServer)
    {
    }

protected:
    void onInit() override
    {
        createWindow(
            wingz::WindowDesc {
                .width = 1280,
                .height = 720,
                .title = m_isServer
                    ? "Wingz Engine — Server"
                    : "Wingz Engine — Client" }
        );

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        glViewport(0, 0, window().width(), window().height());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_spriteBatch = std::make_unique<wingz::gfx::SpriteBatch>();
        m_debugUI = std::make_unique<wingz::gfx::DebugUI>(window().nativeHandle());
        m_inputManager = std::make_unique<wingz::input::InputManager>();
        m_inputManager->attach(window().nativeHandle());
        m_scene = std::make_unique<wingz::Scene>();
        m_scene->init();

        if (m_isServer)
        {
            m_host = wingz::net::Host::createServer(7777);
            m_replication = std::make_unique<wingz::net::ReplicationSystem>();
            m_localPlayerEntity = createPlayer(0);
            createWalls();
            spdlog::info("Режим сервера");
        }
        else
        {
            m_host = wingz::net::Host::createClient();
            m_replication = std::make_unique<wingz::net::ReplicationSystem>();
            m_host->connect("127.0.0.1", 7777);
            spdlog::info("Режим клиента");
        }
    }

    void onUpdate(float dt) override
    {
        m_inputManager->beginFrame();

        auto& snap = m_inputManager->snapshot();
        const auto keyState = snap.keys[static_cast<size_t>(wingz::input::Key::Escape)];
        if (keyState == wingz::input::InputState::Pressed)
        {
            spdlog::info("ESC — выход");
            if (m_host && m_host->isRunning())
                m_host->disconnect();
            glfwSetWindowShouldClose(window().nativeHandle(), GLFW_TRUE);
            return;
        }

        float moveX = 0.0f;
        float moveY = 0.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::A)] >= wingz::input::InputState::Pressed)
            moveX -= 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::D)] >= wingz::input::InputState::Pressed)
            moveX += 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::W)] >= wingz::input::InputState::Pressed)
            moveY -= 1.0f;
        if (snap.keys[static_cast<size_t>(wingz::input::Key::S)] >= wingz::input::InputState::Pressed)
            moveY += 1.0f;

        if (m_localPlayerEntity != entt::null && m_scene->registry().valid(m_localPlayerEntity))
        {
            auto& intent = m_scene->registry().get<wingz::ecs::InputIntent>(m_localPlayerEntity);
            intent.moveX = moveX;
            intent.moveY = moveY;
        }

        ++m_networkTick;
        if (m_host && m_host->isRunning())
        {
            m_host->poll(
                [this](const wingz::net::NetEvent& e)
                { handleNetEvent(e); }
            );

            if (m_isServer)
            {
                if (m_networkTick % 2 == 0)
                {
                    m_scene->update(1.0f / 30.0f);
                    m_replication->serverUpdate(
                        m_scene->registry(),
                        *m_host,
                        m_networkTick
                    );
                }
            }
            else if (m_connected)
            {
                if (m_networkTick % 2 == 0)
                    sendInputToServer(moveX, moveY);
            }
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_scene->render(*m_spriteBatch);

        m_debugUI->beginFrame();
        ImGui::Begin("Network");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Mode: %s", m_isServer ? "Server" : "Client");
        ImGui::Text("Connected: %s", m_connected ? "yes" : "no");
        ImGui::Text("Tick: %u", m_networkTick);

        auto view = m_scene->registry().view<wingz::ecs::Transform, wingz::ecs::Tag, wingz::ecs::Player>();
        for (auto e : view)
        {
            auto& t = view.get<wingz::ecs::Transform>(e);
            auto& tag = view.get<wingz::ecs::Tag>(e);
            auto& p = view.get<wingz::ecs::Player>(e);
            ImGui::Text("  %s (pid=%u): (%.0f, %.0f)", tag.name.c_str(), p.id, t.x, t.y);
        }
        ImGui::End();
        m_debugUI->endFrame();

        m_inputManager->endFrame();
    }

    void onShutdown() override
    {
        if (m_host && m_host->isRunning())
            m_host->disconnect();
    }

private:
    entt::entity createPlayer(wingz::net::PeerId playerId)
    {
        auto e = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(e, 400.0f, 360.0f, 0.0f);
        m_scene->registry().emplace<wingz::ecs::Velocity>(e, 0.0f, 0.0f, 0.0f);
        m_scene->registry().emplace<wingz::ecs::Sprite>(
            e, 0u, 0, 0, 1, 1, 48, 48,
            (playerId == 0) ? 0.2f : 0.2f,
            (playerId == 0) ? 0.6f : 1.0f,
            (playerId == 0) ? 1.0f : 0.2f, 1.0f
        );
        m_scene->registry().emplace<wingz::ecs::Tag>(
            e,
            (playerId == 0) ? "ServerPlayer" : "ClientPlayer"
        );
        m_scene->registry().emplace<wingz::ecs::Player>(e, playerId);
        m_scene->registry().emplace<wingz::ecs::InputIntent>(e, 0, 0, false);
        m_scene->registry().emplace<wingz::physics::Collider>(
            e,
            wingz::physics::AABB { 0, 0, 24, 24 },
            wingz::physics::CollisionLayer::Player,
            wingz::physics::CollisionLayer::Wall | wingz::physics::CollisionLayer::Player,
            false, false
        );
        if (m_replication)
            m_replication->registerEntity(m_scene->registry(), e);
        return e;
    }

    void createWalls()
    {
        auto makeWall = [this](float x, float y, float w, float h, const char* tag)
        {
            auto e = m_scene->registry().create();
            m_scene->registry().emplace<wingz::ecs::Transform>(e, x, y, 0);
            m_scene->registry().emplace<wingz::ecs::Sprite>(
                e, 0u, 0, 0, 1, 1, w, h, 0.8f, 0.3f, 0.3f, 1
            );
            m_scene->registry().emplace<wingz::ecs::Tag>(e, tag);
            m_scene->registry().emplace<wingz::physics::Collider>(
                e,
                wingz::physics::AABB { 0, 0, w * 0.5f, h * 0.5f },
                wingz::physics::CollisionLayer::Wall,
                wingz::physics::CollisionLayer::Player,
                false, true
            );
            if (m_replication)
                m_replication->registerEntity(m_scene->registry(), e);
        };
        makeWall(640, 16, 1280, 32, "WallT");
        makeWall(640, 704, 1280, 32, "WallB");
        makeWall(16, 360, 32, 720, "WallL");
        makeWall(1264, 360, 32, 720, "WallR");
    }

    void sendInputToServer(float mx, float my)
    {
        if (!m_host || !m_connected)
            return;

        wingz::net::InputPacket p { m_networkTick, mx, my, false };
        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::InputState;
        msg.header.tick = m_networkTick;
        msg.data.resize(sizeof(p));
        std::memcpy(msg.data.data(), &p, sizeof(p));
        m_host->send(0, msg, false);
    }

    void handleNetEvent(const wingz::net::NetEvent& e)
    {
        switch (e.type)
        {
        case wingz::net::NetEvent::Type::Connect:
            spdlog::info("Connect peer={}", e.peerId);
            if (m_isServer)
            {
                m_clientPlayerId = e.peerId;
                createPlayer(m_clientPlayerId);
                m_replication->serverUpdate(m_scene->registry(), *m_host, m_networkTick);
            }
            else
                m_connected = true;
            break;
        case wingz::net::NetEvent::Type::Disconnect:
            spdlog::info("Disconnect peer={}", e.peerId);
            if (m_isServer)
            {
                auto v = m_scene->registry().view<wingz::ecs::Player>();
                for (auto en : v)
                    if (v.get<wingz::ecs::Player>(en).id == m_clientPlayerId)
                        m_scene->registry().destroy(en);
                m_clientPlayerId = 0xFFFFFFFF;
            }
            else
                m_connected = false;
            break;
        case wingz::net::NetEvent::Type::Data:
            if (e.message.header.type == wingz::net::MessageType::InputState)
            {
                if (m_isServer && e.message.data.size() >= sizeof(wingz::net::InputPacket))
                {
                    auto* inp = reinterpret_cast<const wingz::net::InputPacket*>(e.message.data.data());
                    auto v = m_scene->registry().view<wingz::ecs::Player, wingz::ecs::InputIntent>();
                    for (auto en : v)
                    {
                        if (v.get<wingz::ecs::Player>(en).id == m_clientPlayerId)
                        {
                            v.get<wingz::ecs::InputIntent>(en).moveX = inp->moveX;
                            v.get<wingz::ecs::InputIntent>(en).moveY = inp->moveY;
                            break;
                        }
                    }
                }
            }
            else if (e.message.header.type == wingz::net::MessageType::WorldState)
            {
                if (!m_isServer && m_replication)
                {
                    m_replication->clientReceive(m_scene->registry(), e.message);
                    if (m_localPlayerEntity == entt::null)
                    {
                        auto v = m_scene->registry().view<wingz::ecs::Player>();
                        for (auto en : v)
                        {
                            if (v.get<wingz::ecs::Player>(en).id != 0)
                            {
                                m_localPlayerEntity = en;
                                m_clientPlayerId = v.get<wingz::ecs::Player>(en).id;
                                spdlog::info("Client found player id={}", m_clientPlayerId);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    bool m_isServer;
    bool m_connected = false;
    wingz::net::PeerId m_clientPlayerId = 0xFFFFFFFF;
    entt::entity m_localPlayerEntity = entt::null;
    wingz::net::TickNumber m_networkTick = 0;

    std::unique_ptr<wingz::gfx::SpriteBatch> m_spriteBatch;
    std::unique_ptr<wingz::gfx::DebugUI> m_debugUI;
    std::unique_ptr<wingz::input::InputManager> m_inputManager;
    std::unique_ptr<wingz::Scene> m_scene;
    std::unique_ptr<wingz::net::Host> m_host;
    std::unique_ptr<wingz::net::ReplicationSystem> m_replication;
};

int main(int argc, char* argv[])
{
    SandboxApp app(argc > 1 && std::strcmp(argv[1], "--server") == 0);
    return app.run(argc, argv);
}
