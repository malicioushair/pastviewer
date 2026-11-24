#include "PositionPathFilter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>

#include <QDateTime>

namespace {
constexpr auto LOOKAHEAD_MS = 1000;
constexpr auto MAX_HOLD_MS = 5000;
constexpr auto MAX_PLAUSIBLE_SPEED = 4.0; // m/s before looking for a spike
constexpr auto MAX_ACCURACY_ALLOWANCE_METERS = 5.0;
constexpr auto MIN_INTERVAL_SECONDS = 0.1;
constexpr auto MILLISECONDS_PER_SECOND = 1000.0;
constexpr auto MIN_SUSPICIOUS_DISTANCE_METERS = 8.0;
constexpr auto MAX_SPIKE_RETURN_RATIO = 0.6;
constexpr auto MIN_CONTINUATION_SEGMENT_METERS = 8.0;
constexpr auto MIN_CONTINUATION_SPEED_RATIO = 0.5;
constexpr auto MAX_CONTINUATION_SPEED_RATIO = 2.0;
constexpr auto MIN_CONTINUATION_STRAIGHTNESS = 0.8;

struct Sample
{
    QGeoPositionInfo position;
    int64_t arrival;
};

double Accuracy(const Sample & sample)
{
    if (!sample.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
        return 0.0;
    const auto value = sample.position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return std::isfinite(value) ? std::clamp(value, 0.0, MAX_ACCURACY_ALLOWANCE_METERS) : 0.0;
}

double Seconds(const Sample & a, const Sample & b)
{
    const auto & first = a.position.timestamp();
    const auto & second = b.position.timestamp();
    const auto millis = first.isValid() && second.isValid() ? first.msecsTo(second) : b.arrival - a.arrival;
    return std::max(MIN_INTERVAL_SECONDS, static_cast<double>(millis) / MILLISECONDS_PER_SECOND);
}

double Distance(const Sample & a, const Sample & b)
{
    return a.position.coordinate().distanceTo(b.position.coordinate());
}

bool Suspicious(const Sample & a, const Sample & b)
{
    const auto distance = Distance(a, b);
    const auto uncertainty = Accuracy(a) + Accuracy(b);
    return true
        && distance > MIN_SUSPICIOUS_DISTANCE_METERS + uncertainty
        && (distance - uncertainty) / Seconds(a, b) > MAX_PLAUSIBLE_SPEED;
}

bool IsSpike(const Sample & a, const Sample & b, const Sample & c)
{
    const auto outward = Distance(a, b);
    const auto back = Distance(b, c);
    const auto displacement = Distance(a, c);
    // A fast excursion followed by a return close to the original path.
    return Suspicious(b, c) && displacement < outward * MAX_SPIKE_RETURN_RATIO
        && displacement < back * MAX_SPIKE_RETURN_RATIO;
}

bool Continues(const Sample & a, const Sample & b, const Sample & c)
{
    const auto first = Distance(a, b);
    const auto second = Distance(b, c);
    const auto speed1 = first / Seconds(a, b);
    const auto speed2 = second / Seconds(b, c);
    return first > MIN_CONTINUATION_SEGMENT_METERS && second > MIN_CONTINUATION_SEGMENT_METERS
        && speed2 >= speed1 * MIN_CONTINUATION_SPEED_RATIO && speed2 <= speed1 * MAX_CONTINUATION_SPEED_RATIO
        && Distance(a, c) >= (first + second) * MIN_CONTINUATION_STRAIGHTNESS;
}

} // namespace

struct PositionPathFilter::Impl
{
    std::deque<Sample> pending;
    std::optional<Sample> accepted;
    std::optional<Sample> previous;
    QDateTime lastTimestamp;
};

PositionPathFilter::PositionPathFilter()
    : m_impl(std::make_unique<Impl>())
{}

PositionPathFilter::~PositionPathFilter() = default;

void PositionPathFilter::Push(const QGeoPositionInfo & position, qint64 arrival)
{
    if (!position.coordinate().isValid()
        || (true
            && position.timestamp().isValid()
            && m_impl->lastTimestamp.isValid()
            && position.timestamp() <= m_impl->lastTimestamp))
        return;

    if (position.timestamp().isValid())
        m_impl->lastTimestamp = position.timestamp();
    m_impl->pending.push_back({ position, arrival });
}

std::optional<QGeoPositionInfo> PositionPathFilter::Take(qint64 now)
{
    while (!m_impl->pending.empty())
    {
        const auto & current = m_impl->pending.front();
        if (m_impl->accepted && now - current.arrival < LOOKAHEAD_MS)
            return std::nullopt;

        if (m_impl->accepted && Suspicious(*m_impl->accepted, current))
        {
            if (m_impl->pending.size() < 2 && !(m_impl->previous && Continues(*m_impl->previous, *m_impl->accepted, current)))
            {
                // Do not freeze indefinitely when updates arrive slowly or stop.
                if (now - current.arrival < MAX_HOLD_MS)
                    return std::nullopt;
            }
            else if (m_impl->pending.size() >= 2 && IsSpike(*m_impl->accepted, current, m_impl->pending[1]))
            {
                m_impl->pending.pop_front();
                continue;
            }
        }
        m_impl->previous = m_impl->accepted;
        m_impl->accepted = current;
        auto result = current.position;
        m_impl->pending.pop_front();
        return result;
    }
    return std::nullopt;
}

bool PositionPathFilter::HasPending() const
{
    return !m_impl->pending.empty();
}
