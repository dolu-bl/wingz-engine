#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "input_manager.h"

namespace wingz::input
{

/// Привязка: действие + комбинация клавиш/кнопок.
struct InputBinding
{
    std::vector<Key> keys;
    std::vector<MouseButton> mouseButtons;
};

/// Маппинг "название действия → клавиши".
/// Проверяет снапшот и вызывает колбэки.
class ActionMap
{
public:
    using ActionCallback = std::function<void()>;

    /// Зарегистрировать действие с привязкой.
    void addAction(const std::string& name, const InputBinding& binding, ActionCallback onPressed);

    /// Проверить снапшот и вызвать колбэки для активных действий.
    void update(const InputSnapshot& snapshot);

private:
    struct ActionEntry
    {
        InputBinding binding;
        ActionCallback onPressed;
        bool wasActive = false;
    };

    std::unordered_map<std::string, ActionEntry> m_actions;
};

} // namespace wingz::input
