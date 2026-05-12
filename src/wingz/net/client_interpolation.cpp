#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include <wingz/net/client_interpolation.h>

namespace wingz::net
{

namespace
{

/// Угловая интерполяция (кратчайший путь по окружности).
inline float lerpAngle(float a, float b, float t)
{
    float diff = b - a;
    // Нормализуем разницу в диапазон [-PI, PI], но у нас углы в радианах без ограничений,
    // поэтому используем простой подход с приведением к диапазону
    while (diff > M_PI)
        diff -= 2.0f * M_PI;
    while (diff < -M_PI)
        diff += 2.0f * M_PI;
    return a + diff * t;
}

} // namespace

struct ClientInterpolation::Impl
{
    /// На сколько тиков отстаём от сервера.
    static constexpr TickNumber kBufferTicks = 2;

    /// Максимальное количество хранимых снапшотов.
    static constexpr size_t kMaxSnapshots = 16;

    /// Снапшот за один тик.
    struct Snapshot
    {
        TickNumber tick = 0;
        std::vector<SerializedEntityState> states;
    };

    /// Очередь снапшотов (новые в конец).
    std::deque<Snapshot> snapshots;

    /// Текущие интерполированные состояния (выход для рендера).
    std::unordered_map<NetEntityId, InterpolatedState> interpolated;

    /// Последний обработанный тик.
    TickNumber lastRenderTick = 0;

    void storeSnapshot(TickNumber tick, const std::vector<SerializedEntityState>& states)
    {
        // Ищем, куда вставить (снапшоты должны быть упорядочены по тику)
        auto it = snapshots.begin();
        while (it != snapshots.end() && it->tick < tick)
            ++it;

        // Не дублируем снапшоты с одинаковым тиком
        if (it != snapshots.end() && it->tick == tick)
        {
            // Обновляем существующий
            it->states = states;
            return;
        }

        Snapshot snap;
        snap.tick = tick;
        snap.states = states;
        snapshots.insert(it, std::move(snap));

        // Удаляем старые, если накопилось слишком много
        while (snapshots.size() > kMaxSnapshots)
            snapshots.pop_front();
    }

    void computeInterpolation(float renderTickFloat)
    {
        if (snapshots.size() < 2)
            return;

        TickNumber renderTick = static_cast<TickNumber>(renderTickFloat);
        float intraTickFraction = renderTickFloat - static_cast<float>(renderTick);

        // Находим два снапшота, между которыми находится renderTick
        const Snapshot* snapA = nullptr;
        const Snapshot* snapB = nullptr;

        for (size_t i = 0; i < snapshots.size() - 1; ++i)
        {
            if (snapshots[i].tick <= renderTick && renderTick <= snapshots[i + 1].tick)
            {
                snapA = &snapshots[i];
                snapB = &snapshots[i + 1];
                break;
            }
        }

        // Если renderTick раньше первого снапшота — используем самый ранний
        if (!snapA && !snapshots.empty() && renderTick <= snapshots.front().tick)
        {
            snapA = &snapshots.front();
            snapB = &snapshots.front();
        }

        // Если renderTick позже последнего — используем самый поздний
        if (!snapA && !snapshots.empty() && renderTick >= snapshots.back().tick)
        {
            snapA = &snapshots.back();
            snapB = &snapshots.back();
        }

        if (!snapA || !snapB)
            return;

        // Вычисляем коэффициент интерполяции
        TickNumber tickRange = snapB->tick - snapA->tick;
        float t = 0.0f;
        if (tickRange > 0)
        {
            t = (static_cast<float>(renderTick - snapA->tick) + intraTickFraction)
                / static_cast<float>(tickRange);
            t = std::clamp(t, 0.0f, 1.0f);
        }

        // Собираем все известные netId из обоих снапшотов
        std::unordered_map<NetEntityId, const SerializedEntityState*> mapA;
        std::unordered_map<NetEntityId, const SerializedEntityState*> mapB;

        for (const auto& st : snapA->states)
            mapA[st.netId] = &st;
        for (const auto& st : snapB->states)
            mapB[st.netId] = &st;

        // Интерполируем
        for (const auto& [netId, stateB] : mapB)
        {
            InterpolatedState& result = interpolated[netId];
            result.active = true;

            auto itA = mapA.find(netId);
            if (itA != mapA.end())
            {
                const auto& stateA = *itA->second;
                result.x = stateA.x + (stateB->x - stateA.x) * t;
                result.y = stateA.y + (stateB->y - stateA.y) * t;
                result.rot = lerpAngle(stateA.rot, stateB->rot, t);
            }
            else
            {
                // Сущность появилась только во втором снапшоте — используем как есть
                result.x = stateB->x;
                result.y = stateB->y;
                result.rot = stateB->rot;
            }
        }

        // Помечаем неактивными сущности, которых нет в снапшоте B
        for (auto& [netId, state] : interpolated)
        {
            if (mapB.find(netId) == mapB.end())
                state.active = false;
        }
    }
};

ClientInterpolation::ClientInterpolation()
    : m_impl(std::make_unique<Impl>())
{
}

ClientInterpolation::~ClientInterpolation() = default;

void ClientInterpolation::pushSnapshot(
    TickNumber tick,
    const std::vector<SerializedEntityState>& states
)
{
    m_impl->storeSnapshot(tick, states);

    spdlog::debug(
        "ClientInterpolation: получен снапшот tick={}, сущностей={}, буфер={}",
        tick,
        states.size(),
        m_impl->snapshots.size()
    );
}

void ClientInterpolation::update(TickNumber currentTick, float dt, float tickDuration)
{
    // Вычисляем тик рендера с отставанием от текущего
    // currentTick — это последний известный тик клиента
    // renderTick отстаёт на kBufferTicks для буферизации
    TickNumber renderTick = (currentTick >= Impl::kBufferTicks)
        ? currentTick - Impl::kBufferTicks
        : 0;

    // Также интерполируем внутри тика для сверхплавности
    // dtFraction: насколько мы продвинулись внутри текущего тика
    float tickFraction = dt / tickDuration;
    float interpolatedTick = static_cast<float>(renderTick) + tickFraction;

    // TODO: в будущем можно интерполировать между целыми тиками,
    // используя interpolatedTick вместо renderTick.
    // Пока для простоты используем целочисленный renderTick.

    float interpolatedTickFloat = static_cast<float>(renderTick) + tickFraction;
    m_impl->computeInterpolation(interpolatedTickFloat);
    m_impl->lastRenderTick = renderTick;

    // Удаляем снапшоты, которые старше renderTick
    while (!m_impl->snapshots.empty()
           && m_impl->snapshots.front().tick < renderTick)
    {
        m_impl->snapshots.pop_front();
    }
}

const InterpolatedState* ClientInterpolation::getState(NetEntityId netId) const
{
    auto it = m_impl->interpolated.find(netId);
    if (it != m_impl->interpolated.end() && it->second.active)
        return &it->second;
    return nullptr;
}

void ClientInterpolation::reset()
{
    m_impl->snapshots.clear();
    m_impl->interpolated.clear();
    m_impl->lastRenderTick = 0;
}

} // namespace wingz::net
