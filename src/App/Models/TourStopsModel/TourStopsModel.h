#pragma once

#include <QAbstractListModel>

#include "App/Models/AvailableToursModel/AvailableToursModel.h" // @TODO move TourStop to its own header

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
    void SelectionChanged(const TourStop & stop);

public: // QAbstractListModel
    int rowCount(const QModelIndex & parent = {}) const final;
    QVariant data(const QModelIndex & index, int role) const final;
    QHash<int, QByteArray> roleNames() const final;

public:
    void SetStops(QList<TourStop> stops);
    bool UpdateStop(int row, const TourStop & stop);
    QString GetSelectedStopTitle() const;
    void SelectNextStop();
    void SelectPrevStop();

private:
    QList<TourStop> m_stops {};
};