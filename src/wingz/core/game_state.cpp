#include <algorithm>

#include <spdlog/spdlog.h>

#include <wingz/core/game_state.h>

namespace wingz::core
{

StateStack::~StateStack()
{
    applyCommands();
    while (!m_stack.empty())
    {
        auto* top = m_stack.back().get();
        top->onExit();
        m_stack.pop_back();
    }
}

void StateStack::push(std::unique_ptr<IGameState> state)
{
    if (!state)
        return;

    PendingCommand cmd;
    cmd.type = Command::Push;
    cmd.state = std::move(state);
    m_commands.push_back(std::move(cmd));
}

void StateStack::pop()
{
    PendingCommand cmd;
    cmd.type = Command::Pop;
    m_commands.push_back(std::move(cmd));
}

void StateStack::replace(std::unique_ptr<IGameState> state)
{
    if (!state)
        return;

    PendingCommand cmd;
    cmd.type = Command::Replace;
    cmd.state = std::move(state);
    m_commands.push_back(std::move(cmd));
}

void StateStack::popTo(const std::string& stateName)
{
    PendingCommand cmd;
    cmd.type = Command::PopTo;
    cmd.targetName = stateName;
    m_commands.push_back(std::move(cmd));
}

void StateStack::clear()
{
    PendingCommand cmd;
    cmd.type = Command::Clear;
    m_commands.push_back(std::move(cmd));
}

void StateStack::applyCommands()
{
    if (m_commands.empty())
        return;

    // Применяем все накопленные команды
    for (auto& cmd : m_commands)
    {
        switch (cmd.type)
        {
        case Command::Push:
        {
            if (!m_stack.empty())
            {
                auto* top = m_stack.back().get();
                spdlog::debug("StateStack: пауза '{}'", top->name());
                top->onPause();
            }

            spdlog::info("StateStack: вход в '{}'", cmd.state->name());
            cmd.state->m_ctx = &m_context;
            cmd.state->onEnter(m_context);
            m_stack.push_back(std::move(cmd.state));
            break;
        }

        case Command::Pop:
        {
            if (m_stack.empty())
                break;

            auto* top = m_stack.back().get();
            spdlog::info("StateStack: выход из '{}'", top->name());
            top->onExit();
            m_stack.pop_back();

            if (!m_stack.empty())
            {
                auto* newTop = m_stack.back().get();
                spdlog::debug("StateStack: восстановление '{}'", newTop->name());
                newTop->onResume();
            }
            break;
        }

        case Command::Replace:
        {
            if (!m_stack.empty())
            {
                auto* oldTop = m_stack.back().get();
                spdlog::info("StateStack: замена '{}' -> '{}'", oldTop->name(), cmd.state->name());
                oldTop->onExit();
                m_stack.pop_back();
            }
            else
            {
                spdlog::info("StateStack: вход в '{}' (замена пустого стека)", cmd.state->name());
            }

            cmd.state->m_ctx = &m_context;
            cmd.state->onEnter(m_context);
            m_stack.push_back(std::move(cmd.state));
            break;
        }

        case Command::PopTo:
        {
            while (!m_stack.empty())
            {
                auto* top = m_stack.back().get();
                bool found = (top->name() == cmd.targetName);

                spdlog::info("StateStack: выход из '{}' (popTo '{}')", top->name(), cmd.targetName);
                top->onExit();
                m_stack.pop_back();

                if (found)
                {
                    if (!m_stack.empty())
                    {
                        auto* newTop = m_stack.back().get();
                        spdlog::debug("StateStack: восстановление '{}'", newTop->name());
                        newTop->onResume();
                    }
                    break;
                }
            }
            break;
        }

        case Command::Clear:
        {
            while (!m_stack.empty())
            {
                auto* top = m_stack.back().get();
                spdlog::debug("StateStack: очистка '{}'", top->name());
                top->onExit();
                m_stack.pop_back();
            }
            break;
        }
        }
    }

    m_commands.clear();
}

void StateStack::update(float dt)
{
    // Сначала применяем отложенные команды
    applyCommands();

    // Обновляем сверху вниз
    for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
    {
        auto* state = it->get();
        state->onUpdate(dt);

        if (state->isModal())
            break;
    }
}

void StateStack::render()
{
    size_t startIndex = 0;
    for (size_t i = 0; i < m_stack.size(); ++i)
    {
        if (m_stack[i]->isModal())
        {
            startIndex = i;
            break;
        }
    }

    for (size_t i = startIndex; i < m_stack.size(); ++i)
        m_stack[i]->onRender();
}

size_t StateStack::size() const
{
    return m_stack.size();
}

bool StateStack::empty() const
{
    return m_stack.empty();
}

std::string StateStack::topName() const
{
    if (m_stack.empty())
        return "<empty>";
    return m_stack.back()->name();
}

void StateStack::setContext(const StateContext& ctx)
{
    m_context = ctx;
}

StateContext& StateStack::context()
{
    return m_context;
}

const StateContext& StateStack::context() const
{
    return m_context;
}

} // namespace wingz::core
