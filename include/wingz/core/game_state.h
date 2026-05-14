#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <wingz/gfx/imgui_context.h>
#include <wingz/gfx/sprite_batch.h>
#include <wingz/input/input_manager.h>
#include <wingz/net/host.h>
#include <wingz/net/replication.h>
#include <wingz/window.h>

namespace wingz::core
{

// Предварительные декларации
class StateStack;

/// Контекст, передаваемый состоянию при входе.
/// Содержит ссылки на системы движка, нужные состоянию.
struct StateContext
{
    // Ссылка на стек состояний (чтобы состояние могло push/pop)
    StateStack* stack = nullptr;

    // Ссылки на основные системы (заполняются при создании)
    class Window* window = nullptr;
    class gfx::SpriteBatch* spriteBatch = nullptr;
    class input::InputManager* inputManager = nullptr;
    class Scene* scene = nullptr;
    class net::Host* host = nullptr;
    class net::ReplicationSystem* replication = nullptr;
    class gfx::ImGuiContext* imGui = nullptr;
};

/// Базовый класс игрового состояния (экрана, сцены).
///
/// Игра состоит из стека таких состояний.
/// Примеры: MainMenuState, GameplayState, SettingsState, LoadingState.
///
/// Каждое состояние получает управление через методы onEnter/onUpdate/onRender/onExit.
/// Состояние может быть:
/// - Прозрачным (isModal() == false): нижележащие состояния тоже обновляются/рендерятся
/// - Модальным (isModal() == true): только это состояние активно
class IGameState
{
public:
    virtual ~IGameState() = default;

    /// Вызывается при добавлении состояния в стек.
    /// Здесь можно инициализировать ресурсы, создать UI, запустить музыку.
    /// @param ctx Контекст с ссылками на системы движка.
    virtual void onEnter(StateContext& ctx) { }

    /// Вызывается при удалении состояния из стека.
    /// Здесь освобождают ресурсы, сохраняют данные.
    virtual void onExit() { }

    /// Вызывается когда состояние временно скрыто (поверх него добавили другое).
    /// Полезно для паузы игры при открытии меню.
    virtual void onPause() { }

    /// Вызывается когда состояние снова становится верхним.
    virtual void onResume() { }

    /// Обновление логики. Вызывается каждый кадр.
    /// @param dt Время с последнего кадра в секундах.
    virtual void onUpdate(float dt) = 0;

    /// Отрисовка. Вызывается каждый кадр.
    /// Состояние само решает, что и как рендерить.
    virtual void onRender() = 0;

    /// Должно ли это состояние блокировать обновление и рендер нижних.
    /// true — только это состояние активно (например, главное меню).
    /// false — прозрачное (например, внутриигровое меню паузы, HUD).
    virtual bool isModal() const { return true; }

    /// Имя состояния для отладки.
    virtual std::string name() const { return "UnknownState"; }

protected:
    /// Удобный доступ к контексту (сохраняется при onEnter).
    StateContext* ctx() { return m_ctx; }
    const StateContext* ctx() const { return m_ctx; }

private:
    friend class StateStack;
    StateContext* m_ctx = nullptr;
};

/// Стек игровых состояний.
///
/// Управляет жизненным циклом состояний.
/// Самые верхние состояния получают управление первыми.
///
/// Пример использования:
/// @code
/// stack.push(std::make_unique<MainMenuState>());
/// // ...
/// stack.push(std::make_unique<GameplayState>());
/// // ...
/// stack.pop(); // вернуться в меню
/// @endcode
class StateStack
{
public:
    /// Тип отложенной операции.
    enum class Command
    {
        Push,
        Pop,
        Replace,
        PopTo,
        Clear
    };

    StateStack() = default;
    ~StateStack();

    StateStack(const StateStack&) = delete;
    StateStack& operator=(const StateStack&) = delete;

    // ────────────────────────────────────────
    // Эти методы можно вызывать в любое время (в том числе из onUpdate/onRender).
    // Операции выполняются отложенно, после завершения текущего кадра.
    // ────────────────────────────────────────

    void push(std::unique_ptr<IGameState> state);
    void pop();
    void replace(std::unique_ptr<IGameState> state);
    void popTo(const std::string& stateName);
    void clear();

    // ────────────────────────────────────────
    // Вызываются движком. Не трогать из игры.
    // ────────────────────────────────────────

    void update(float dt);
    void render();

    // ────────────────────────────────────────
    // Информация
    // ────────────────────────────────────────

    size_t size() const;
    bool empty() const;
    std::string topName() const;

    void setContext(const StateContext& ctx);
    StateContext& context();
    const StateContext& context() const;

private:
    /// Применить все накопленные команды.
    void applyCommands();

    struct PendingCommand
    {
        Command type;
        std::unique_ptr<IGameState> state;
        std::string targetName;
    };

    StateContext m_context;
    std::vector<std::unique_ptr<IGameState>> m_stack;
    std::vector<PendingCommand> m_commands;
};

} // namespace wingz::core
