#pragma once

#include <memory>
#include <optional>

#include <QGeoPositionInfo>

// Arrival times are monotonic milliseconds. Position timestamps are used for
// speed when available; arrival times are the fallback.
class PositionPathFilter
{
public:
    PositionPathFilter();
    ~PositionPathFilter();

    void Push(const QGeoPositionInfo & position, qint64 arrival);
    std::optional<QGeoPositionInfo> Take(qint64 now);
    bool HasPending() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
