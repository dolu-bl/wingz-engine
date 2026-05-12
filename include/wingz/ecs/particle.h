#pragma once

#include <cstdint>

namespace wingz::ecs
{

/// Компонент частицы.
/// Сущность с этим компонентом автоматически управляется ParticleSystem.
struct Particle
{
    /// Оставшееся время жизни в секундах.
    float lifetime = 1.0f;

    /// Прошедшее время с момента создания.
    float elapsed = 0.0f;

    /// Начальный цвет (RGBA).
    float startR = 1.0f, startG = 1.0f, startB = 1.0f, startA = 1.0f;

    /// Конечный цвет (для затухания/изменения цвета).
    float endR = 1.0f, endG = 1.0f, endB = 1.0f, endA = 0.0f;

    /// Начальный размер.
    float startWidth = 8.0f;
    float startHeight = 8.0f;

    /// Конечный размер.
    float endWidth = 8.0f;
    float endHeight = 8.0f;

    /// Затухает ли частица (прозрачность к концу).
    bool fadeOut = true;

    /// Мерцание (случайное изменение прозрачности).
    bool flicker = false;

    /// Тип частицы (для разных шейдеров/сортировки в будущем).
    enum class Type : uint8_t
    {
        Default,
        Spark,
        Smoke,
        Explosion
    };
    Type type = Type::Default;
};

/// Эмиттер частиц.
/// Создаёт новые частицы с заданными параметрами.
struct ParticleEmitter
{
    /// Оставшееся время до следующего спавна.
    float spawnTimer = 0.0f;

    /// Интервал между спавнами в секундах.
    float spawnInterval = 0.1f;

    /// Сколько частиц создавать за раз.
    uint32_t burstCount = 1;

    /// Максимальное количество одновременно живых частиц от этого эмиттера.
    uint32_t maxParticles = 100;

    /// Базовое время жизни новых частиц.
    float baseLifetime = 1.0f;

    /// Разброс времени жизни (±).
    float lifetimeVariance = 0.0f;

    /// Начальная скорость (базовая).
    float baseSpeed = 100.0f;

    /// Разброс скорости (±).
    float speedVariance = 50.0f;

    /// Угол разлёта в радианах (0 = направленный, PI*2 = круговой).
    float spreadAngle = 6.283185f; // 2*PI — во все стороны

    /// Базовое направление (в радианах, 0 = вправо).
    float baseAngle = 0.0f;

    /// Начальный цвет.
    float startR = 1.0f, startG = 1.0f, startB = 1.0f, startA = 1.0f;

    /// Конечный цвет.
    float endR = 1.0f, endG = 1.0f, endB = 1.0f, endA = 0.0f;

    /// Начальный размер.
    float startWidth = 8.0f, startHeight = 8.0f;

    /// Конечный размер.
    float endWidth = 4.0f, endHeight = 4.0f;

    /// Тип создаваемых частиц.
    Particle::Type particleType = Particle::Type::Default;

    /// Затухание.
    bool fadeOut = true;

    /// Мерцание.
    bool flicker = false;

    /// Однократный выброс (true) или постоянный эмиттер (false).
    bool oneShot = false;

    /// Активен ли эмиттер.
    bool active = true;

    /// Сколько частиц уже создано (для oneShot).
    uint32_t spawnedCount = 0;

    /// Альтернатива: при одном выбросе создать все частицы сразу.
    uint32_t totalBurstCount = 0;
};

} // namespace wingz::ecs
