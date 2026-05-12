#pragma once

#include <cstdint>
#include <string>

namespace wingz::ecs
{

/// Позиция и поворот в 2D.
struct Transform
{
    float x = 0.0f;
    float y = 0.0f;
    float rot = 0.0f;
};

/// Скорость движения.
struct Velocity
{
    float dx = 0.0f;
    float dy = 0.0f;
    float drot = 0.0f;
};

/// Спрайт (указатель на текстуру и её регион).
struct Sprite
{
    uint32_t textureId = 0;
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 1.0f, v1 = 1.0f;
    float width = 64.0f;
    float height = 64.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

/// Тег (имя сущности для отладки).
struct Tag
{
    std::string name;
};

/// Компонент игрока (для идентификации).
struct Player
{
    uint8_t id = 0;
};

/// Входной компонент (направление движения).
struct InputIntent
{
    float moveX = 0.0f; // -1, 0, +1
    float moveY = 0.0f;
    bool fire = false;
};

} // namespace wingz::ecs
