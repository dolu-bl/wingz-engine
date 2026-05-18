#pragma once

#include <memory>

#include <wingz/core/asset_manager.h>
#include <wingz/core/game_state.h>

namespace wingz
{

class Window;

namespace gfx
{
class ImGuiContext;
}

namespace input
{
class InputManager;
}

/// Базовый класс для приложения на Wingz Engine.
/// Игра наследуется от App и реализует onInit/onUpdate/onShutdown.
class App
{
public:
    App();
    virtual ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Точка входа. Вызывается из main(). Возвращает код завершения.
    int run(int argc, char** argv);

protected:
    /// Вызывается один раз при старте приложения.
    /// Игра должна создать окно через createWindow().
    virtual void onInit() = 0;

    /// Вызывается каждый кадр. dt — время кадра в секундах.
    virtual void onUpdate(float dt) { };

    /// Вызывается при завершении приложения.
    virtual void onShutdown() = 0;

    /// Создаёт окно и сохраняет его внутри App.
    void createWindow(const struct WindowDesc& desc);

    /// Доступ к окну приложения.
    Window& window();

    void createInput();
    void createImGui();

    core::StateStack& stateStack();
    virtual core::StateContext createContext();

    /// Создать AssetManager. Вызови в onInit() если нужен.
    void createAssetManager();

    /// Доступ к AssetManager.
    core::AssetManager& assets();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz
