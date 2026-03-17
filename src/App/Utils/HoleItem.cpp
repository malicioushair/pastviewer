#include "HoleItem.h"

#include <QHoverEvent>
#include <QMouseEvent>
#include <QWheelEvent>

namespace PastViewer {

namespace {

QList<QRectF> variantListToRectFs(const QVariantList & list)
{
	QList<QRectF> result;
	for (const QVariant & v : list)
	{
		if (v.canConvert<QRectF>())
			result.append(v.toRectF());
		else if (v.canConvert<QVariantMap>())
		{
			const auto m = v.toMap();
			result.append(QRectF(
				m.value("x", 0).toReal(),
				m.value("y", 0).toReal(),
				m.value("width", 0).toReal(),
				m.value("height", 0).toReal()));
		}
	}
	return result;
}

}

HoleItem::HoleItem(QQuickItem * parent)
	: QQuickItem(parent)
{
	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::AllButtons);
}

QVariantList HoleItem::holes() const
{
	QVariantList list;
	for (const auto & r : m_holes)
		list.append(r);
	return list;
}

void HoleItem::setHoles(const QVariantList & list)
{
	const auto newHoles = variantListToRectFs(list);
	if (newHoles == m_holes)
		return;

	m_holes = newHoles;
	emit holesChanged();
}

bool HoleItem::isInsideHole(const QPointF & point) const
{
	return std::ranges::any_of(m_holes, [point](const QRectF & hole) { return hole.contains(point); });
}

bool HoleItem::contains(const QPointF & point) const
{
	return !isInsideHole(point);
}

bool HoleItem::filterMouseEvent(QMouseEvent * event) const
{
	const auto filtered = !isInsideHole(event->position());
	event->setAccepted(filtered);
	return filtered;
}

void HoleItem::hoverEnterEvent(QHoverEvent * event)
{
	QQuickItem::hoverEnterEvent(event);
}

void HoleItem::hoverMoveEvent(QHoverEvent * event)
{
	QQuickItem::hoverMoveEvent(event);
}

void HoleItem::hoverLeaveEvent(QHoverEvent * event)
{
	QQuickItem::hoverLeaveEvent(event);
}

void HoleItem::mousePressEvent(QMouseEvent * event)
{
	(void)filterMouseEvent(event);
}

void HoleItem::mouseMoveEvent(QMouseEvent * event)
{
	(void)filterMouseEvent(event);
}

void HoleItem::mouseReleaseEvent(QMouseEvent * event)
{
	(void)filterMouseEvent(event);
}

void HoleItem::mouseDoubleClickEvent(QMouseEvent * event)
{
	(void)filterMouseEvent(event);
}

void HoleItem::wheelEvent(QWheelEvent * event)
{
	event->accept();
}

}
