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
#include <wingz/ecs/particle.h>
#include <wingz/ecs/particle_system.h>
#include <wingz/gfx/camera.h>
#include <wingz/gfx/debug_ui.h>
#include <wingz/gfx/sprite_batch.h>
#include <wingz/gfx/texture.h>
#include <wingz/input/action_map.h>
#include <wingz/input/input_manager.h>
#include <wingz/net/client_interpolation.h>
#include <wingz/net/host.h>
#include <wingz/net/replication.h>
#include <wingz/net/serializer.h>
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

        // ────────────────────────────────────────────
        // Эмиттеры частиц — создаются и на сервере, и на клиенте
        // ────────────────────────────────────────────

        // Эмиттер искр (центр экрана)
        {
            auto emitterEntity = m_scene->registry().create();
            m_scene->registry().emplace<wingz::ecs::Transform>(emitterEntity, 640.0f, 360.0f, 0.0f);

            wingz::ecs::ParticleEmitter emitter;
            emitter.spawnInterval = 0.02f;
            emitter.burstCount = 2;
            emitter.baseLifetime = 0.8f;
            emitter.lifetimeVariance = 0.3f;
            emitter.baseSpeed = 80.0f;
            emitter.speedVariance = 40.0f;
            emitter.spreadAngle = 0.5f;
            emitter.baseAngle = -3.14159265f / 2.0f;
            emitter.startR = 1.0f;
            emitter.startG = 0.6f;
            emitter.startB = 0.0f;
            emitter.startA = 1.0f;
            emitter.endR = 1.0f;
            emitter.endG = 0.2f;
            emitter.endB = 0.0f;
            emitter.endA = 0.0f;
            emitter.startWidth = 6.0f;
            emitter.startHeight = 6.0f;
            emitter.endWidth = 2.0f;
            emitter.endHeight = 2.0f;
            emitter.fadeOut = true;
            emitter.flicker = true;
            emitter.particleType = wingz::ecs::Particle::Type::Spark;
            emitter.active = true;

            m_scene->registry().emplace<wingz::ecs::ParticleEmitter>(emitterEntity, emitter);
            m_scene->registry().emplace<wingz::ecs::Tag>(emitterEntity, "SparkEmitter");
        }

        // Эмиттер дыма
        {
            auto emitterEntity = m_scene->registry().create();
            m_scene->registry().emplace<wingz::ecs::Transform>(emitterEntity, 640.0f, 360.0f, 0.0f);

            wingz::ecs::ParticleEmitter emitter;
            emitter.spawnInterval = 0.1f;
            emitter.burstCount = 1;
            emitter.baseLifetime = 2.0f;
            emitter.lifetimeVariance = 0.5f;
            emitter.baseSpeed = 20.0f;
            emitter.speedVariance = 10.0f;
            emitter.spreadAngle = 0.8f;
            emitter.baseAngle = -3.14159265f / 2.0f;
            emitter.startR = 0.5f;
            emitter.startG = 0.5f;
            emitter.startB = 0.5f;
            emitter.startA = 0.6f;
            emitter.endR = 0.3f;
            emitter.endG = 0.3f;
            emitter.endB = 0.3f;
            emitter.endA = 0.0f;
            emitter.startWidth = 16.0f;
            emitter.startHeight = 16.0f;
            emitter.endWidth = 32.0f;
            emitter.endHeight = 32.0f;
            emitter.fadeOut = true;
            emitter.flicker = false;
            emitter.particleType = wingz::ecs::Particle::Type::Smoke;
            emitter.active = true;

            m_scene->registry().emplace<wingz::ecs::ParticleEmitter>(emitterEntity, emitter);
            m_scene->registry().emplace<wingz::ecs::Tag>(emitterEntity, "SmokeEmitter");
        }

        // ────────────────────────────────────────────
        // Сеть
        // ────────────────────────────────────────────

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
            m_interpolation = std::make_unique<wingz::net::ClientInterpolation>();
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

        // Тест: стрельба по пробелу
        if (snap.keys[static_cast<size_t>(wingz::input::Key::Space)]
            == wingz::input::InputState::Pressed)
        {
            sendShootEvent();
        }

        // Тест: взрыв частиц по клику левой кнопкой мыши
        if (snap.mouseButtons[static_cast<size_t>(wingz::input::MouseButton::Left)]
            == wingz::input::InputState::Pressed)
        {
            float worldX = static_cast<float>(snap.mouseX);
            float worldY = static_cast<float>(snap.mouseY);

            // Отправляем событие взрыва через сеть
            sendParticleBurst(worldX, worldY);

            // Локально создаём частицы сразу (для мгновенного отклика)
            spawnExplosion(worldX, worldY);
        }

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
                // Сервер: полный цикл (ввод, движение, частицы, физика)
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
                // Клиент: только визуальные системы (частицы)
                m_scene->updateVisuals(dt);

                if (m_networkTick % 2 == 0)
                    sendInputToServer(moveX, moveY);
            }
        }

        // ────────────────────────────────────────────
        // Обновление интерполяции (только клиент)
        // Используем последний серверный тик, а не локальный
        // ────────────────────────────────────────────
        if (m_interpolation && m_connected && m_lastServerTick > 0)
        {
            // Рендерим с отставанием на 2 тика ОТ ПОСЛЕДНЕГО СЕРВЕРНОГО ТИКА
            m_interpolation->update(m_lastServerTick, dt, 1.0f / 30.0f);

            // Копируем интерполированные позиции в ecs для рендера
            auto view = m_scene->registry().view<wingz::ecs::Transform, wingz::net::Networked>();
            for (auto entity : view)
            {
                const auto& net = view.get<wingz::net::Networked>(entity);
                const auto* istate = m_interpolation->getState(net.netId);

                if (istate)
                {
                    auto& itransform = m_scene->registry().get_or_emplace<wingz::ecs::InterpolatedTransform>(entity);
                    itransform.x = istate->x;
                    itransform.y = istate->y;
                    itransform.rot = istate->rot;
                }
            }
        }

        // ────────────────────────────────────────────
        // Рендер
        // ────────────────────────────────────────────
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_scene->render(*m_spriteBatch);

        // ────────────────────────────────────────────
        // Отладочный UI
        // ────────────────────────────────────────────
        m_debugUI->beginFrame();
        ImGui::Begin("Network");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Mode: %s", m_isServer ? "Server" : "Client");
        ImGui::Text("Connected: %s", m_connected ? "yes" : "no");
        ImGui::Text("Local tick: %u", m_networkTick);
        ImGui::Text("Server tick: %u", m_lastServerTick);
        ImGui::Text("Interp lag: %d ticks", static_cast<int32_t>(m_lastServerTick) - static_cast<int32_t>(m_networkTick));

        auto view = m_scene->registry().view<wingz::ecs::Transform, wingz::ecs::Tag, wingz::ecs::Player>();
        for (auto e : view)
        {
            auto& t = view.get<wingz::ecs::Transform>(e);
            auto& tag = view.get<wingz::ecs::Tag>(e);
            auto& p = view.get<wingz::ecs::Player>(e);

            float interpX = t.x;
            float interpY = t.y;
            const auto* it = m_scene->registry().try_get<wingz::ecs::InterpolatedTransform>(e);
            if (it)
            {
                interpX = it->x;
                interpY = it->y;
            }

            ImGui::Text("  %s (pid=%u): raw(%.0f, %.0f) interp(%.0f, %.0f)", tag.name.c_str(), p.id, t.x, t.y, interpX, interpY);
        }

        // Статистика частиц
        auto particleView = m_scene->registry().view<wingz::ecs::Particle>();
        ImGui::Text("Particles alive: %zu", particleView.size());

        auto emitterView = m_scene->registry().view<wingz::ecs::ParticleEmitter>();
        ImGui::Text("Emitters active: %zu", emitterView.size());

        // Статистика боя
        auto bulletView = m_scene->registry().view<wingz::ecs::Bullet>();
        auto healthView = m_scene->registry().view<wingz::ecs::Health>();
        ImGui::Text("Bullets: %zu", bulletView.size());
        ImGui::Text("Destructible objects: %zu", healthView.size());

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
        auto makeWall = [this](float x, float y, float w, float h, const char* tag, bool destructible)
        {
            auto e = m_scene->registry().create();
            m_scene->registry().emplace<wingz::ecs::Transform>(e, x, y, 0);
            m_scene->registry().emplace<wingz::ecs::Sprite>(
                e, 0u, 0, 0, 1, 1, w, h,
                destructible ? 0.6f : 0.8f, // R: разрушаемые чуть темнее
                destructible ? 0.4f : 0.3f, // G
                destructible ? 0.2f : 0.3f, // B
                1.0f
            );
            m_scene->registry().emplace<wingz::ecs::Tag>(e, tag);
            m_scene->registry().emplace<wingz::physics::Collider>(
                e,
                wingz::physics::AABB { 0, 0, w * 0.5f, h * 0.5f },
                wingz::physics::CollisionLayer::Wall,
                wingz::physics::CollisionLayer::Player
                    | wingz::physics::CollisionLayer::PlayerBullet,
                false,
                true // статик
            );

            // Разрушаемые стены получают здоровье
            if (destructible)
            {
                m_scene->registry().emplace<wingz::ecs::Health>(
                    e, 100.0f, 100.0f
                );
            }

            if (m_replication)
                m_replication->registerEntity(m_scene->registry(), e);
        };

        // Внешние стены — неразрушимые
        makeWall(640, 16, 1280, 32, "WallT", false);
        makeWall(640, 704, 1280, 32, "WallB", false);
        makeWall(16, 360, 32, 720, "WallL", false);
        makeWall(1264, 360, 32, 720, "WallR", false);

        // Внутренние разрушаемые стены для теста
        makeWall(400, 300, 32, 120, "DestructibleWall1", true);
        makeWall(600, 400, 120, 32, "DestructibleWall2", true);
        makeWall(800, 250, 32, 100, "DestructibleWall3", true);
        makeWall(350, 500, 100, 32, "DestructibleWall4", true);
    }

    void sendInputToServer(float mx, float my)
    {
        if (!m_host || !m_connected)
            return;

        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::InputState;
        msg.header.tick = m_networkTick;

        wingz::net::Serializer ser(msg.data);
        ser.writeU32(m_networkTick);
        ser.writeF32(mx);
        ser.writeF32(my);
        ser.writeU8(0); // fire = false

        m_host->send(0, msg, false);
    }

    void sendParticleBurst(float worldX, float worldY)
    {
        if (!m_host || !m_host->isRunning())
            return;

        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::ParticleBurst;
        msg.header.tick = m_networkTick;

        wingz::net::Serializer ser(msg.data);
        ser.writeF32(worldX);
        ser.writeF32(worldY);

        if (m_isServer)
        {
            // Сервер сразу рассылает всем клиентам
            m_host->broadcast(msg, false);
        }
        else if (m_connected)
        {
            // Клиент отправляет серверу
            m_host->send(0, msg, false);
        }
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
        {
            if (e.message.header.type == wingz::net::MessageType::InputState)
            {
                if (m_isServer)
                {
                    wingz::net::Serializer ser(e.message.data);
                    if (ser.remainingBytes() >= sizeof(uint32_t) + sizeof(float) * 2 + sizeof(uint8_t))
                    {
                        uint32_t tick = ser.readU32();
                        float mx = ser.readF32();
                        float my = ser.readF32();
                        uint8_t fire = ser.readU8();

                        auto v = m_scene->registry().view<wingz::ecs::Player, wingz::ecs::InputIntent>();
                        for (auto en : v)
                        {
                            if (v.get<wingz::ecs::Player>(en).id == m_clientPlayerId)
                            {
                                v.get<wingz::ecs::InputIntent>(en).moveX = mx;
                                v.get<wingz::ecs::InputIntent>(en).moveY = my;

                                if (fire)
                                {
                                    // Временно сохраняем localPlayerEntity для spawnBullet
                                    auto savedLocal = m_localPlayerEntity;
                                    m_localPlayerEntity = en;
                                    spawnBullet();
                                    m_localPlayerEntity = savedLocal;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else if (e.message.header.type == wingz::net::MessageType::WorldState)
            {
                if (!m_isServer && m_replication)
                {
                    // Сохраняем последний серверный тик ДО интерполяции
                    m_lastServerTick = e.message.header.tick;

                    // Передаём снапшот в интерполятор
                    if (m_interpolation)
                    {
                        auto states = m_replication->deserializeWorldState(e.message);
                        m_interpolation->pushSnapshot(e.message.header.tick, states);
                    }

                    // Применяем состояние напрямую к ecs (для коллизий и физики клиента)
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
            else if (e.message.header.type == wingz::net::MessageType::ParticleBurst)
            {
                // Взрыв от другого игрока
                wingz::net::Serializer ser(e.message.data);
                if (ser.remainingBytes() >= sizeof(float) * 2)
                {
                    float x = ser.readF32();
                    float y = ser.readF32();
                    spawnExplosion(x, y);
                }

                // Сервер пересылает взрыв всем остальным клиентам
                if (m_isServer)
                {
                    m_host->broadcast(e.message, false, e.peerId);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    void spawnExplosion(float x, float y)
    {
        wingz::ecs::ParticleEmitter explosionEmitter;
        explosionEmitter.baseLifetime = 0.6f;
        explosionEmitter.lifetimeVariance = 0.2f;
        explosionEmitter.baseSpeed = 200.0f;
        explosionEmitter.speedVariance = 100.0f;
        explosionEmitter.spreadAngle = 6.283185f;
        explosionEmitter.baseAngle = 0.0f;
        explosionEmitter.startR = 1.0f;
        explosionEmitter.startG = 0.8f;
        explosionEmitter.startB = 0.2f;
        explosionEmitter.startA = 1.0f;
        explosionEmitter.endR = 1.0f;
        explosionEmitter.endG = 0.1f;
        explosionEmitter.endB = 0.0f;
        explosionEmitter.endA = 0.0f;
        explosionEmitter.startWidth = 10.0f;
        explosionEmitter.startHeight = 10.0f;
        explosionEmitter.endWidth = 2.0f;
        explosionEmitter.endHeight = 2.0f;
        explosionEmitter.fadeOut = true;
        explosionEmitter.flicker = true;
        explosionEmitter.particleType = wingz::ecs::Particle::Type::Explosion;

        wingz::ecs::ParticleSystem::emitBurst(
            m_scene->registry(),
            x, y,
            explosionEmitter,
            30
        );
    }

    void sendShootEvent()
    {
        if (!m_host || !m_host->isRunning())
            return;

        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::InputState; // можно завести отдельный тип
        msg.header.tick = m_networkTick;

        wingz::net::Serializer ser(msg.data);
        ser.writeU32(m_networkTick);
        ser.writeF32(0.0f); // moveX не используется
        ser.writeF32(0.0f); // moveY не используется
        ser.writeU8(1); // fire = true

        if (m_isServer)
        {
            // Сервер сразу создаёт пулю
            spawnBullet();
            // И рассылает событие клиентам (опционально)
        }
        else
        {
            m_host->send(0, msg, false);
        }
    }

    void spawnBullet()
    {
        if (m_localPlayerEntity == entt::null
            || !m_scene->registry().valid(m_localPlayerEntity))
            return;

        auto& playerTransform = m_scene->registry().get<wingz::ecs::Transform>(
            m_localPlayerEntity
        );

        auto bullet = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(
            bullet, playerTransform.x, playerTransform.y - 20.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Velocity>(bullet, 0.0f, -500.0f, 0.0f);
        m_scene->registry().emplace<wingz::ecs::Sprite>(
            bullet, 0u, 0.0f, 0.0f, 1.0f, 1.0f, 8.0f, 12.0f,
            1.0f, 1.0f, 0.0f, 1.0f
        );
        m_scene->registry().emplace<wingz::ecs::Tag>(bullet, "Bullet");
        m_scene->registry().emplace<wingz::ecs::Bullet>(
            bullet, 25.0f, m_localPlayerEntity
        );
        m_scene->registry().emplace<wingz::physics::Collider>(
            bullet,
            wingz::physics::AABB { 0, 0, 4, 6 },
            wingz::physics::CollisionLayer::PlayerBullet,
            wingz::physics::CollisionLayer::Wall,
            false, false
        );

        // Регистрируем для репликации
        if (m_replication)
            m_replication->registerEntity(m_scene->registry(), bullet);
    }

    // Интерполяция
    wingz::net::TickNumber m_lastServerTick = 0; // последний тик, пришедший от сервера
    std::unique_ptr<wingz::net::ClientInterpolation> m_interpolation;

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
