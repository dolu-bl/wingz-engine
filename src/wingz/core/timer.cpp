#include <algorithm>

#include <wingz/core/timer.h>

namespace wingz::core
{

Timer::Timer(float duration)
{
    start(duration);
}

void Timer::start(float duration)
{
    m_duration = std::max(0.0f, duration);
    m_elapsed = 0.0f;
    m_running = true;
    m_finished = false;
}

void Timer::stop()
{
    m_running = false;
}

bool Timer::update(float dt)
{
    if (!m_running)
        return false;

    if (m_finished)
        return false;

    m_elapsed += dt;

    if (m_elapsed >= m_duration)
    {
        m_elapsed = m_duration;
        m_running = false;
        m_finished = true;
        return true;
    }

    return false;
}

float Timer::progress() const
{
    if (m_duration <= 0.0f)
        return 1.0f;

    return std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
}

float Timer::remaining() const
{
    if (!m_running || m_finished)
        return 0.0f;

    return std::max(0.0f, m_duration - m_elapsed);
}

bool Timer::isRunning() const
{
    return m_running && !m_finished;
}

bool Timer::isFinished() const
{
    return m_finished;
}

float Timer::duration() const
{
    return m_duration;
}

float Timer::elapsed() const
{
    return m_elapsed;
}

} // namespace wingz::core
