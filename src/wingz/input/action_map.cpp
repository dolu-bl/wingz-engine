#include <spdlog/spdlog.h>

#include "wingz/input/action_map.h"

namespace wingz::input
{

void ActionMap::addAction(const std::string& name, const InputBinding& binding, ActionCallback onPressed)
{
    m_actions[name] = {
        binding,
        std::move(onPressed),
        false
    };
}

void ActionMap::update(const InputSnapshot& snapshot)
{
    for (auto& [name, entry] : m_actions)
    {
        bool active = false;

        for (auto key : entry.binding.keys)
        {
            auto state = snapshot.keys[static_cast<size_t>(key)];
            if (state == InputState::Pressed || state == InputState::Held)
            {
                active = true;
                break;
            }
        }

        if (!active)
        {
            for (auto mb : entry.binding.mouseButtons)
            {
                auto state = snapshot.mouseButtons[static_cast<size_t>(mb)];
                if (state == InputState::Pressed || state == InputState::Held)
                {
                    active = true;
                    break;
                }
            }
        }

        // Вызываем коллбэк, пока действие активно (для удержания)
        if (active && entry.onPressed)
        {
            spdlog::debug("Действие '{}' активно", name);
            entry.onPressed();
        }

        entry.wasActive = active;
    }
}

} // namespace wingz::input
