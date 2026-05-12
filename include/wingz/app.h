#pragma once

#include <memory>

namespace wingz
{

class Window;

/// Базовый класс для приложения на Wingz Engine.
/// Игра наследуется от App и реализует onInit/onUpdate/onShutdown.
class App
{
public:
    App();
    virtual ~App();

    // Запрещаем копирование
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Точка входа. Вызывается из main(). Возвращает код завершения.
    int run(int argc, char** argv);

protected:
    /// Вызывается один раз при старте приложения.
    /// Игра должна создать окно через createWindow().
    virtual void onInit() = 0;

    /// Вызывается каждый кадр. dt — время кадра в секундах.
    virtual void onUpdate(float dt) = 0;

    /// Вызывается при завершении приложения.
    virtual void onShutdown() = 0;

    /// Создаёт окно и сохраняет его внутри App.
    void createWindow(const struct WindowDesc& desc);

    /// Доступ к окну приложения.
    Window& window();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace wingz
