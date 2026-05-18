#include <cstring>
#include <memory>
#include <stdexcept>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <spdlog/spdlog.h>

#include <wingz/app.h>
#include <wingz/core/game_state.h>
#include <wingz/core/timer.h>
#include <wingz/ecs/components.h>
#include <wingz/ecs/particle.h>
#include <wingz/ecs/particle_system.h>
#include <wingz/gfx/camera.h>
#include <wingz/gfx/camera_controller.h>
#include <wingz/gfx/imgui_context.h>
#include <wingz/gfx/sprite_batch.h>
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

class GameplayState : public wingz::core::IGameState
{
public:
    explicit GameplayState(bool isServer)
        : m_isServer(isServer)
    {
    }

    void onEnter(wingz::core::StateContext& ctx) override
    {
        m_window = ctx.window;
        m_spriteBatch = std::make_unique<wingz::gfx::SpriteBatch>();
        m_scene = std::make_unique<wingz::Scene>();
        m_scene->init();

        m_scene->cameraController().setWorldBounds(-500.0f, 1780.0f, -500.0f, 1220.0f);

        setupCameraActions();
        setupEmitters();

        if (m_isServer)
        {
            m_host = wingz::net::Host::createServer(7777);
            m_replication = std::make_unique<wingz::net::ReplicationSystem>();
            m_localPlayerEntity = createPlayer(0);
            createWalls();
            m_scene->setHitCallback([this](float x, float y)
                                    { sendHitEffect(x, y); });
            spdlog::info("Режим сервера");
        }
        else
        {
            m_host = wingz::net::Host::createClient();
            m_replication = std::make_unique<wingz::net::ReplicationSystem>();
            m_replication->setAssetManager(ctx.assets);
            m_host->connect("127.0.0.1", 7777);
            m_interpolation = std::make_unique<wingz::net::ClientInterpolation>();
            spdlog::info("Режим клиента");
        }
    }

    void onUpdate(float dt) override
    {
        auto* input = ctx()->inputManager;
        input->beginFrame();
        auto& snap = input->snapshot();

        // Выход по Escape
        if (snap.keys[static_cast<size_t>(wingz::input::Key::Escape)]
            == wingz::input::InputState::Pressed)
        {
            spdlog::info("ESC — выход в меню");
            if (m_host && m_host->isRunning())
                m_host->disconnect();
            ctx()->stack->pop();
            input->endFrame();
            return;
        }

        // Движение игрока
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

        // Стрельба
        if (snap.keys[static_cast<size_t>(wingz::input::Key::Space)]
            >= wingz::input::InputState::Pressed)
        {
            if (!m_shootCooldown.isRunning())
            {
                sendShootEvent();
                m_shootCooldown.start(0.25f);
            }
        }
        m_shootCooldown.update(dt);

        // Взрыв по клику
        if (snap.mouseButtons[static_cast<size_t>(wingz::input::MouseButton::Left)]
            == wingz::input::InputState::Pressed)
        {
            float worldX = static_cast<float>(snap.mouseX);
            float worldY = static_cast<float>(snap.mouseY);
            sendParticleBurst(worldX, worldY);
            spawnExplosion(worldX, worldY);
        }

        // Переключение камеры
        if (snap.keys[static_cast<size_t>(wingz::input::Key::F1)]
            == wingz::input::InputState::Pressed)
        {
            m_scene->cameraController().toggleMode();
        }

        m_cameraActionMap.update(snap);

        // Сеть
        ++m_networkTick;
        if (m_host && m_host->isRunning())
        {
            m_host->poll([this](const wingz::net::NetEvent& e)
                         { handleNetEvent(e); });

            if (m_isServer)
            {
                if (m_networkTick % 2 == 0)
                {
                    m_scene->update(1.0f / 30.0f);
                    m_replication->serverUpdate(m_scene->registry(), *m_host, m_networkTick);
                }
            }
            else if (m_connected)
            {
                m_scene->updateVisuals(dt);
                if (m_networkTick % 2 == 0)
                    sendInputToServer(moveX, moveY);
            }
        }

        // Интерполяция
        if (m_interpolation && m_connected && m_lastServerTick > 0)
        {
            m_interpolation->update(m_lastServerTick, dt, 1.0f / 30.0f);

            auto view = m_scene->registry().view<wingz::ecs::Transform, wingz::net::Networked>();
            for (auto entity : view)
            {
                const auto& net = view.get<wingz::net::Networked>(entity);
                const auto* istate = m_interpolation->getState(net.netId);
                if (istate)
                {
                    auto& it = m_scene->registry()
                                   .get_or_emplace<wingz::ecs::InterpolatedTransform>(entity);
                    it.x = istate->x;
                    it.y = istate->y;
                    it.rot = istate->rot;
                }
            }
        }

        // Камера
        float targetX = 640.0f;
        float targetY = 360.0f;
        if (m_localPlayerEntity != entt::null && m_scene->registry().valid(m_localPlayerEntity))
        {
            const auto& pt = m_scene->registry().get<wingz::ecs::Transform>(m_localPlayerEntity);
            targetX = pt.x;
            targetY = pt.y;
        }
        m_scene->cameraController().update(dt, targetX, targetY);

        input->endFrame();
    }

    void onRender() override
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_scene->render(*m_spriteBatch);

        if (ctx()->imGui)
        {
            ctx()->imGui->beginFrame();

            ImGui::Begin("Network");
            ImGui::Text("Mode: %s", m_isServer ? "Server" : "Client");
            ImGui::Text("Connected: %s", m_connected ? "yes" : "no");
            ImGui::Text("Tick: %u", m_networkTick);
            ImGui::Text("Server tick: %u", m_lastServerTick);
            ImGui::Text("Camera: %s", m_scene->cameraController().isFollowing() ? "Follow" : "Free");

            auto particleView = m_scene->registry().view<wingz::ecs::Particle>();
            auto bulletView = m_scene->registry().view<wingz::ecs::Bullet>();
            auto healthView = m_scene->registry().view<wingz::ecs::Health>();
            ImGui::Text("Particles: %zu", particleView.size());
            ImGui::Text("Bullets: %zu", bulletView.size());
            ImGui::Text("Destructible: %zu", healthView.size());

            auto view = m_scene->registry()
                            .view<wingz::ecs::Transform, wingz::ecs::Tag, wingz::ecs::Player>();
            for (auto e : view)
            {
                auto& t = view.get<wingz::ecs::Transform>(e);
                auto& tag = view.get<wingz::ecs::Tag>(e);
                auto& p = view.get<wingz::ecs::Player>(e);

                float interpX = t.x;
                float interpY = t.y;
                const auto* it = m_scene->registry()
                                     .try_get<wingz::ecs::InterpolatedTransform>(e);
                if (it)
                {
                    interpX = it->x;
                    interpY = it->y;
                }

                ImGui::Text("  %s (pid=%u): raw(%.0f, %.0f) interp(%.0f, %.0f)", tag.name.c_str(), p.id, t.x, t.y, interpX, interpY);
            }
            ImGui::End();

            ctx()->imGui->endFrame();
        }
    }

    void onExit() override
    {
        if (m_host && m_host->isRunning())
            m_host->disconnect();
    }

    std::string name() const override { return "GameplayState"; }

private:
    void setupCameraActions()
    {
        m_cameraActionMap.addAction(
            "CameraLeft",
            wingz::input::InputBinding {
                { wingz::input::Key::Left, wingz::input::Key::H }, {} },
            [this]()
            { m_scene->cameraController().moveLeft(); }
        );

        m_cameraActionMap.addAction(
            "CameraRight",
            wingz::input::InputBinding {
                { wingz::input::Key::Right, wingz::input::Key::K }, {} },
            [this]()
            { m_scene->cameraController().moveRight(); }
        );

        m_cameraActionMap.addAction(
            "CameraUp",
            wingz::input::InputBinding {
                { wingz::input::Key::Up, wingz::input::Key::U }, {} },
            [this]()
            { m_scene->cameraController().moveUp(); }
        );

        m_cameraActionMap.addAction(
            "CameraDown",
            wingz::input::InputBinding {
                { wingz::input::Key::Down, wingz::input::Key::J }, {} },
            [this]()
            { m_scene->cameraController().moveDown(); }
        );
    }

    void setupEmitters()
    {
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
    }

    entt::entity createPlayer(wingz::net::PeerId playerId)
    {
        auto e = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(e, 400.0f, 360.0f, 0.0f);
        m_scene->registry().emplace<wingz::ecs::Velocity>(e, 0.0f, 0.0f, 0.0f);

        // Получаем спрайт из атласа
        wingz::ecs::Sprite sprite;
        sprite.textureId = 0;
        sprite.width = 48.0f;
        sprite.height = 48.0f;
        sprite.r = (playerId == 0) ? 1.0f : 1.0f;
        sprite.g = (playerId == 0) ? 1.0f : 0.7f;
        sprite.b = (playerId == 0) ? 1.0f : 0.7f;
        sprite.a = 1.0f;

        // Получаем ресурс из AssetManager по имени
        const auto* resource = ctx()->assets->getResource("player_ship");
        if (resource)
        {
            sprite.resourceId = resource->id;
            sprite.textureId = resource->textureHandle;
            sprite.u0 = resource->u0;
            sprite.v0 = resource->v0;
            sprite.u1 = resource->u1;
            sprite.v1 = resource->v1;
            sprite.width = resource->width;
            sprite.height = resource->height;
        }

        m_scene->registry().emplace<wingz::ecs::Sprite>(e, sprite);
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

            // Спрайт стены
            wingz::ecs::Sprite sprite;
            sprite.textureId = 0;
            sprite.width = w;
            sprite.height = h;
            sprite.r = destructible ? 0.6f : 0.8f;
            sprite.g = destructible ? 0.6f : 0.8f;
            sprite.b = destructible ? 0.6f : 0.8f;
            sprite.a = 1.0f;

            // Выбираем имя региона в зависимости от ориентации стены
            std::string regionName = (std::abs(h) < std::abs(w)) ? "wall_top" : "wall_side";
            const auto* resource = ctx()->assets->getResource(regionName);
            if (resource)
            {
                sprite.resourceId = resource->id;
                sprite.textureId = resource->textureHandle;
                sprite.u0 = resource->u0;
                sprite.v0 = resource->v0;
                sprite.u1 = resource->u1;
                sprite.v1 = resource->v1;
            }

            m_scene->registry().emplace<wingz::ecs::Sprite>(e, sprite);
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
        ser.writeU8(0);

        m_host->send(0, msg, false);
    }

    void sendShootEvent()
    {
        if (!m_host || !m_host->isRunning())
            return;

        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::InputState;
        msg.header.tick = m_networkTick;

        wingz::net::Serializer ser(msg.data);
        ser.writeU32(m_networkTick);
        ser.writeF32(0.0f);
        ser.writeF32(0.0f);
        ser.writeU8(1);

        if (m_isServer)
        {
            spawnBullet();
        }
        else
        {
            m_host->send(0, msg, false);
        }
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
            m_host->broadcast(msg, false);
        }
        else if (m_connected)
        {
            m_host->send(0, msg, false);
        }
    }

    void sendHitEffect(float x, float y)
    {
        if (!m_host || !m_host->isRunning() || !m_isServer)
            return;

        spawnHitParticles(x, y);

        wingz::net::Message msg;
        msg.header.type = wingz::net::MessageType::HitEffect;
        msg.header.tick = m_networkTick;

        wingz::net::Serializer ser(msg.data);
        ser.writeF32(x);
        ser.writeF32(y);

        m_host->broadcast(msg, false);
    }

    void spawnBullet()
    {
        if (m_localPlayerEntity == entt::null
            || !m_scene->registry().valid(m_localPlayerEntity))
            return;

        auto& playerTransform = m_scene->registry()
                                    .get<wingz::ecs::Transform>(m_localPlayerEntity);

        auto bullet = m_scene->registry().create();
        m_scene->registry().emplace<wingz::ecs::Transform>(
            bullet, playerTransform.x, playerTransform.y - 20.0f, 0.0f
        );
        m_scene->registry().emplace<wingz::ecs::Velocity>(
            bullet, 0.0f, -500.0f, 0.0f
        );

        // Спрайт пули
        wingz::ecs::Sprite sprite;
        sprite.textureId = 0;
        sprite.width = 8.0f;
        sprite.height = 12.0f;
        sprite.r = 1.0f;
        sprite.g = 1.0f;
        sprite.b = 1.0f;
        sprite.a = 1.0f;

        const auto* resource = ctx()->assets->getResource("bullet");
        if (resource)
        {
            sprite.resourceId = resource->id;
            sprite.textureId = resource->textureHandle;
            sprite.u0 = resource->u0;
            sprite.v0 = resource->v0;
            sprite.u1 = resource->u1;
            sprite.v1 = resource->v1;
            sprite.width = resource->width;
            sprite.height = resource->height;
        }

        m_scene->registry().emplace<wingz::ecs::Sprite>(bullet, sprite);
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

        if (m_replication)
            m_replication->registerEntity(m_scene->registry(), bullet);
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
            m_scene->registry(), x, y, explosionEmitter, 30
        );
    }

    void spawnHitParticles(float x, float y)
    {
        wingz::ecs::ParticleEmitter debrisEmitter;
        debrisEmitter.baseLifetime = 0.4f;
        debrisEmitter.lifetimeVariance = 0.2f;
        debrisEmitter.baseSpeed = 150.0f;
        debrisEmitter.speedVariance = 80.0f;
        debrisEmitter.spreadAngle = 3.14159265f;
        debrisEmitter.baseAngle = 0.0f;
        debrisEmitter.startR = 0.8f;
        debrisEmitter.startG = 0.3f;
        debrisEmitter.startB = 0.3f;
        debrisEmitter.startA = 1.0f;
        debrisEmitter.endR = 0.6f;
        debrisEmitter.endG = 0.2f;
        debrisEmitter.endB = 0.2f;
        debrisEmitter.endA = 0.0f;
        debrisEmitter.startWidth = 6.0f;
        debrisEmitter.startHeight = 6.0f;
        debrisEmitter.endWidth = 2.0f;
        debrisEmitter.endHeight = 2.0f;
        debrisEmitter.fadeOut = true;
        debrisEmitter.flicker = true;
        debrisEmitter.particleType = wingz::ecs::Particle::Type::Spark;

        wingz::ecs::ParticleSystem::emitBurst(
            m_scene->registry(), x, y, debrisEmitter, 12
        );
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
                m_replication->serverUpdate(
                    m_scene->registry(), *m_host, m_networkTick
                );
            }
            else
            {
                m_connected = true;
            }
            break;

        case wingz::net::NetEvent::Type::Disconnect:
            spdlog::info("Disconnect peer={}", e.peerId);
            if (m_isServer)
            {
                auto v = m_scene->registry().view<wingz::ecs::Player>();
                for (auto en : v)
                {
                    if (v.get<wingz::ecs::Player>(en).id == m_clientPlayerId)
                    {
                        m_scene->registry().destroy(en);
                        break;
                    }
                }
                m_clientPlayerId = 0xFFFFFFFF;
            }
            else
            {
                m_connected = false;
            }
            break;

        case wingz::net::NetEvent::Type::Data:
        {
            if (e.message.header.type == wingz::net::MessageType::InputState)
            {
                if (m_isServer)
                {
                    wingz::net::Serializer ser(e.message.data);
                    if (ser.remainingBytes() >= sizeof(uint32_t)
                            + sizeof(float) * 2 + sizeof(uint8_t))
                    {
                        ser.readU32();
                        float mx = ser.readF32();
                        float my = ser.readF32();
                        uint8_t fire = ser.readU8();

                        auto v = m_scene->registry()
                                     .view<wingz::ecs::Player, wingz::ecs::InputIntent>();
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
            else if (e.message.header.type == wingz::net::MessageType::HitEffect)
            {
                // Только клиент создаёт частицы по этому сообщению
                if (!m_isServer)
                {
                    wingz::net::Serializer ser(e.message.data);
                    if (ser.remainingBytes() >= sizeof(float) * 2)
                    {
                        float x = ser.readF32();
                        float y = ser.readF32();
                        spawnHitParticles(x, y);
                    }
                }
            }
            break;
        }

        default:
            break;
        }
    }

    bool m_isServer;
    bool m_connected = false;
    wingz::net::PeerId m_clientPlayerId = 0xFFFFFFFF;
    entt::entity m_localPlayerEntity = entt::null;
    wingz::net::TickNumber m_networkTick = 0;
    wingz::net::TickNumber m_lastServerTick = 0;

    wingz::Window* m_window = nullptr;
    std::unique_ptr<wingz::gfx::SpriteBatch> m_spriteBatch;
    std::unique_ptr<wingz::Scene> m_scene;
    std::unique_ptr<wingz::net::Host> m_host;
    std::unique_ptr<wingz::net::ReplicationSystem> m_replication;
    std::unique_ptr<wingz::net::ClientInterpolation> m_interpolation;
    wingz::input::ActionMap m_cameraActionMap;
    wingz::core::Timer m_shootCooldown;
};

class MainMenuState : public wingz::core::IGameState
{
public:
    void onUpdate(float dt) override
    {
        auto* input = ctx()->inputManager;
        input->beginFrame();
        auto& snap = input->snapshot();

        if (snap.keys[static_cast<size_t>(wingz::input::Key::Enter)]
            == wingz::input::InputState::Pressed)
        {
            ctx()->stack->replace(std::make_unique<GameplayState>(true));
        }

        if (snap.keys[static_cast<size_t>(wingz::input::Key::C)]
            == wingz::input::InputState::Pressed)
        {
            ctx()->stack->replace(std::make_unique<GameplayState>(false));
        }

        if (snap.keys[static_cast<size_t>(wingz::input::Key::Escape)]
            == wingz::input::InputState::Pressed)
        {
            ctx()->stack->pop();
        }

        input->endFrame();
    }

    void onRender() override
    {
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (ctx()->imGui)
        {
            ctx()->imGui->beginFrame();

            ImGui::Begin("Wingz Engine");
            ImGui::Text("Main Menu");
            ImGui::Separator();
            ImGui::Text("Press ENTER to start as Server");
            ImGui::Text("Press C to start as Client");
            ImGui::Text("Press ESC to quit");
            ImGui::End();

            ctx()->imGui->endFrame();
        }
    }

    std::string name() const override { return "MainMenuState"; }
};

class SandboxApp : public wingz::App
{
protected:
    void onInit() override
    {
        createWindow(
            wingz::WindowDesc {
                .width = 1280, .height = 720, .title = "Wingz Engine" }
        );

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Не удалось загрузить OpenGL через glad");

        glViewport(0, 0, window().width(), window().height());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        createInput();
        createImGui();
        createAssetManager();

        // Загрузка ресурсов
        if (!assets().loadManifest("assets/manifest.json"))
        {
            throw std::runtime_error("Не удалось загрузить манифест");
        }

        auto ctx = createContext();
        stateStack().setContext(ctx);
        stateStack().push(std::make_unique<MainMenuState>());
    }

    void onShutdown() override
    {
        stateStack().clear();
    }
};

int main(int argc, char* argv[])
{
    SandboxApp app;
    return app.run(argc, argv);
}
