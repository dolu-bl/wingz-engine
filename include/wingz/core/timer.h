#pragma once

namespace wingz::core
{

/// Простой таймер обратного отсчёта.
///
/// Пример использования:
/// @code
/// Timer shootCooldown;
/// shootCooldown.start(0.25f);  // 250 мс между выстрелами
/// // ...
/// if (shootCooldown.update(dt))
///     fire();
/// @endcode
class Timer
{
public:
    Timer() = default;
    explicit Timer(float duration);

    /// Запустить таймер на duration секунд.
    /// Если таймер уже запущен — перезапускает.
    void start(float duration);

    /// Остановить таймер, не вызывая finished.
    void stop();

    /// Обновить таймер. Уменьшает оставшееся время на dt.
    /// @return true, если время вышло именно в этом кадре (однократный сигнал).
    bool update(float dt);

    /// Прогресс от 0.0 (только запущен) до 1.0 (время вышло).
    /// Возвращает 1.0, если таймер не запущен или остановлен.
    float progress() const;

    /// Оставшееся время в секундах.
    float remaining() const;

    /// Запущен ли таймер.
    bool isRunning() const;

    /// Вышло ли время (не сбрасывается автоматически).
    bool isFinished() const;

    /// Общая длительность, установленная при запуске.
    float duration() const;

    /// Прошедшее время с момента запуска.
    float elapsed() const;

private:
    float m_duration = 0.0f;
    float m_elapsed = 0.0f;
    bool m_running = false;
    bool m_finished = false;
};

} // namespace wingz::core
