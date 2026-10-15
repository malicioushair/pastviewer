#pragma once

#include <QGeoCoordinate>
#include <QObject>

struct DisplayTourStop
{
    Q_GADGET

public:
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QString audioUrl MEMBER audioUrl)
    Q_PROPERTY(QGeoCoordinate coords MEMBER coords)
    Q_PROPERTY(int sequenceNumber MEMBER sequenceNumber)

    bool operator==(const DisplayTourStop & other) const
    {
        return title == other.title
            && description == other.description
            && imageUrl == other.imageUrl
            && audioUrl == other.audioUrl
            && coords == other.coords
            && sequenceNumber == other.sequenceNumber;
    }

    QString title {};
    QString description {};
    QString imageUrl {};
    QString audioUrl {};
    QGeoCoordinate coords {};
    int sequenceNumber {};
    bool selected {};
};
Q_DECLARE_METATYPE(DisplayTourStop)

struct DisplayTour
{
    Q_GADGET

public:
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QList<QGeoCoordinate> tourPath MEMBER tourPath)
    Q_PROPERTY(QList<DisplayTourStop> stops MEMBER stops)
    Q_PROPERTY(int distance MEMBER distance)
    Q_PROPERTY(int stopCount MEMBER stopCount)

    QString title {};
    QString description {};
    QString imageUrl {};
    QList<QGeoCoordinate> tourPath {};
    QList<DisplayTourStop> stops {};
    int distance {};
    int stopCount {};
};
Q_DECLARE_METATYPE(DisplayTour)
