#pragma once

#include <QAbstractListModel>

#include "App/Tours/DisplayTours.h"

class TourStopsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        Title = Qt::UserRole + 1,
        Description,
        ImageUrl,
        Coords,
        SequenceNumber,
        Selected,
    };

    explicit TourStopsModel(QObject * parent = nullptr);

signals:
    void SelectionChanged(const DisplayTourStop & stop);

public: // QAbstractListModel
    int rowCount(const QModelIndex & parent = {}) const final;
    QVariant data(const QModelIndex & index, int role) const final;
    QHash<int, QByteArray> roleNames() const final;

public:
    void SetStops(QList<DisplayTourStop> stops);
    bool UpdateStop(int row, const DisplayTourStop & stop);
    QString GetSelectedStopTitle() const;
    void SelectNextStop();
    void SelectPrevStop();

private:
    QList<DisplayTourStop> m_stops {};
};