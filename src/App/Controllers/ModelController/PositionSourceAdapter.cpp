#include "PositionSourceAdapter.h"

#include <cmath>
#include <limits>

#include <QCompass>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QTimer>

#include "PositionPathFilter.h"

namespace {

// 5m is just an arbitrary number to prevent dirrection jitter
constexpr auto MIN_DISTANCE_METERS = 5.0;
constexpr auto COMPASS_TIMEOUT_MS = 5000;
}

struct PositionSourceAdapter::Impl
{
    Impl(const QGeoPositionInfoSource * const source)
        : source(source)
    {}

    const QGeoPositionInfoSource * const source;
    QGeoPositionInfo position;
    PositionPathFilter positionFilter;
    QElapsedTimer positionClock;
    QTimer positionTimer;
    QGeoCoordinate lastValidCoordinate;
    QGeoCoordinate previousCoordinate;
    double bearing { std::numeric_limits<double>::quiet_NaN() };
    double movementBearing { std::numeric_limits<double>::quiet_NaN() };
    double compassBearing { std::numeric_limits<double>::quiet_NaN() };
    QCompass compass;
    QTimer compassTimeout;
    bool positionAvailable { true };
};

PositionSourceAdapter::PositionSourceAdapter(const QGeoPositionInfoSource * const source, QObject * parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>(source))
{
    m_impl->positionClock.start();
    m_impl->positionTimer.setInterval(200);
    connect(&m_impl->positionTimer, &QTimer::timeout, this, &PositionSourceAdapter::DrainPositionBuffer);
    if (source)
        connect(m_impl->source, &QGeoPositionInfoSource::positionUpdated, this, &PositionSourceAdapter::OnPositionUpdated);

    m_impl->compassTimeout.setSingleShot(true);
    connect(&m_impl->compassTimeout, &QTimer::timeout, this, [this] {
        m_impl->compassBearing = std::numeric_limits<double>::quiet_NaN();
        UpdateBearing();
    });
    connect(&m_impl->compass, &QCompass::readingChanged, this, [this] {
        const auto * reading = m_impl->compass.reading();
        if (!reading || !std::isfinite(reading->azimuth()))
            return;
        m_impl->compassBearing = reading->azimuth();
        m_impl->compassTimeout.start(COMPASS_TIMEOUT_MS);
        UpdateBearing();
    });
    if (auto * app = qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
    {
        connect(app, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive)
                m_impl->compass.start();
            else
            {
                m_impl->compass.stop();
                m_impl->compassTimeout.stop();
                m_impl->compassBearing = std::numeric_limits<double>::quiet_NaN();
                UpdateBearing();
            }
        });
        if (app->applicationState() == Qt::ApplicationActive)
            m_impl->compass.start();
    }
}

PositionSourceAdapter::~PositionSourceAdapter() = default;

QGeoPositionInfo PositionSourceAdapter::Position() const
{
    return m_impl->position;
}

QGeoCoordinate PositionSourceAdapter::Coordinate() const
{
    const auto currentCoord = Position().coordinate();
    return currentCoord.isValid() ? currentCoord : m_impl->lastValidCoordinate;
}

double PositionSourceAdapter::Bearing() const
{
    return m_impl->bearing;
}

bool PositionSourceAdapter::IsPositionAvailable() const
{
    return m_impl->positionAvailable;
}

bool PositionSourceAdapter::IsSourceAvailable() const
{
    return !!m_impl->source;
}

void PositionSourceAdapter::OnPositionUpdated(const QGeoPositionInfo & info)
{
    if (!info.coordinate().isValid())
    {
        m_impl->positionAvailable = false;
        emit PositionAvailableChanged();
        return;
    }
    m_impl->positionFilter.Push(info, m_impl->positionClock.elapsed());
    DrainPositionBuffer();
    if (m_impl->positionFilter.HasPending() && !m_impl->positionTimer.isActive())
        m_impl->positionTimer.start();
}

void PositionSourceAdapter::DrainPositionBuffer()
{
    while (auto position = m_impl->positionFilter.Take(m_impl->positionClock.elapsed()))
        AcceptPosition(*position);
    if (!m_impl->positionFilter.HasPending())
        m_impl->positionTimer.stop();
}

void PositionSourceAdapter::AcceptPosition(const QGeoPositionInfo & info)
{
    const auto currentCoord = info.coordinate();

    if (currentCoord.isValid())
    {
        m_impl->positionAvailable = true;
        emit PositionAvailableChanged();

        if (m_impl->previousCoordinate.isValid())
        {
            const auto distance = m_impl->previousCoordinate.distanceTo(currentCoord);

            if (distance >= MIN_DISTANCE_METERS)
            {
                m_impl->movementBearing = m_impl->previousCoordinate.azimuthTo(currentCoord);
                m_impl->previousCoordinate = currentCoord;
            }
        }
        else
        {
            m_impl->previousCoordinate = currentCoord;
        }

        m_impl->lastValidCoordinate = currentCoord;
        m_impl->position = info;
        if (info.hasAttribute(QGeoPositionInfo::Direction)
            && std::isfinite(info.attribute(QGeoPositionInfo::Direction)))
            m_impl->movementBearing = info.attribute(QGeoPositionInfo::Direction);
        UpdateBearing();
        emit PositionChanged();
        emit PositionAccepted(info);
    }
    else
    {
        m_impl->positionAvailable = false;
        emit PositionAvailableChanged();
    }
}

void PositionSourceAdapter::UpdateBearing()
{
    auto bearing = m_impl->movementBearing;
    if (std::isfinite(m_impl->compassBearing))
    {
        bearing = m_impl->compassBearing;
        if (m_impl->position.hasAttribute(QGeoPositionInfo::MagneticVariation))
        {
            const auto variation = m_impl->position.attribute(QGeoPositionInfo::MagneticVariation);
            if (std::isfinite(variation))
                bearing += variation;
        }
    }
    if (std::isfinite(bearing))
        bearing = std::fmod(std::fmod(bearing, 360.0) + 360.0, 360.0);
    if ((std::isnan(bearing) && std::isnan(m_impl->bearing)) || bearing == m_impl->bearing)
        return;
    m_impl->bearing = bearing;
    emit BearingChanged();
}
