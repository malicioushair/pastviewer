#pragma once

#include <QGeoCoordinate>
#include <QObject>
#include <QStringList>
#include <QtCore/qobject.h>

struct PurchaseInfo
{
    Q_GADGET

public:
    Q_PROPERTY(QString appleId MEMBER appleId)
    Q_PROPERTY(QString googleId MEMBER googleId)
    Q_PROPERTY(bool paid MEMBER paid)

    bool operator==(const PurchaseInfo & other) const
    {
        return true
            && appleId == other.appleId
            && googleId == other.googleId
            && paid == other.paid;
    };

    QString appleId {}; // @TODO: do we reaaly need to know both ids?
    QString googleId {};
    bool paid {};
};
Q_DECLARE_METATYPE(PurchaseInfo)

struct DisplayTourStop
{
    Q_GADGET

public:
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QStringList imageUrls MEMBER imageUrls)
    Q_PROPERTY(QString audioUrl MEMBER audioUrl)
    Q_PROPERTY(QGeoCoordinate coords MEMBER coords)
    Q_PROPERTY(int sequenceNumber MEMBER sequenceNumber)

    bool operator==(const DisplayTourStop & other) const
    {
        return true
            && title == other.title
            && description == other.description
            && imageUrls == other.imageUrls
            && audioUrl == other.audioUrl
            && coords == other.coords
            && sequenceNumber == other.sequenceNumber;
    }

    QString title {};
    QString description {};
    QStringList imageUrls {};
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
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QList<QGeoCoordinate> tourPath MEMBER tourPath)
    Q_PROPERTY(QList<DisplayTourStop> stops MEMBER stops)
    Q_PROPERTY(PurchaseInfo purchaseInfo MEMBER purchaseInfo)
    Q_PROPERTY(int distance MEMBER distance)
    Q_PROPERTY(int stopCount MEMBER stopCount)
    Q_PROPERTY(QString lang MEMBER lang)

    QString id {};
    QString title {};
    QString description {};
    QString imageUrl {};
    QList<QGeoCoordinate> tourPath {};
    QList<DisplayTourStop> stops {};
    PurchaseInfo purchaseInfo {};
    int distance {};
    int stopCount {};
    QString lang {};
};
Q_DECLARE_METATYPE(DisplayTour)
