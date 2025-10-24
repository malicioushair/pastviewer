#include "PositionSourceAdapter.h"

#include <memory>

struct PositionSourceAdapter::Impl
{
	Impl(const QGeoPositionInfoSource & source)
		: source(source)
	{}

	const QGeoPositionInfoSource & source;
	QGeoPositionInfo position;
};

PositionSourceAdapter::PositionSourceAdapter(const QGeoPositionInfoSource & source, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(source))
{
	connect(&m_impl->source, &QGeoPositionInfoSource::positionUpdated, this, &PositionSourceAdapter::OnPositionUpdated);
}

PositionSourceAdapter::~PositionSourceAdapter() = default;

QGeoPositionInfo PositionSourceAdapter::Position() const
{
	return m_impl->position;
}

QGeoCoordinate PositionSourceAdapter::Coordinate() const
{
	return Position().coordinate();
}

void PositionSourceAdapter::OnPositionUpdated(const QGeoPositionInfo & info)
{
	m_impl->position = info;
	emit PositionChanged();
}
