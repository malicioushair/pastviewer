#pragma once

#include <QQuickItem>
#include <QVariantList>

namespace PastViewer {

class HoleItem
	: public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(QVariantList holes READ holes WRITE setHoles NOTIFY holesChanged)

public:
	explicit HoleItem(QQuickItem * parent = nullptr);

	QVariantList holes() const;
	void setHoles(const QVariantList & holes);

signals:
	void holesChanged();

protected:
	bool contains(const QPointF & point) const override;
	void hoverEnterEvent(QHoverEvent * event) override;
	void hoverMoveEvent(QHoverEvent * event) override;
	void hoverLeaveEvent(QHoverEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseDoubleClickEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent * event) override;

private:
	bool isInsideHole(const QPointF & point) const;
	bool filterMouseEvent(QMouseEvent * event) const;

	QList<QRectF> m_holes;
};

}
