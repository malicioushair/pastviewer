#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QGeoCoordinate>

class QNetworkReply;

struct TourStop
{
    Q_GADGET

public:
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QGeoCoordinate coords MEMBER coords)
    Q_PROPERTY(int sequenceNumber MEMBER sequenceNumber)

    bool operator==(const TourStop & other) const
    {
        return title == other.title
            && description == other.description
            && imageUrl == other.imageUrl
            && coords == other.coords
            && sequenceNumber == other.sequenceNumber;
    }

    QString title {};
    QString description {};
    QString imageUrl {};
    QGeoCoordinate coords {};
    int sequenceNumber {};
    bool selected {};
};
Q_DECLARE_METATYPE(TourStop)

struct Tour
{
    Q_GADGET

public:
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QList<QGeoCoordinate> tourPath MEMBER tourPath)
    Q_PROPERTY(QList<TourStop> stops MEMBER stops)
    Q_PROPERTY(int distance MEMBER distance)
    Q_PROPERTY(int stopCount MEMBER stopCount)

    QString title {};
    QString description {};
    QString imageUrl {};
    QList<QGeoCoordinate> tourPath {};
    QList<TourStop> stops {};
    int distance {};
    int stopCount {};
};
Q_DECLARE_METATYPE(Tour)

class AvailableToursModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
    enum Roles
    {
        Title = Qt::UserRole + 1,
        Description,
        ImageUrl,
        Distance,
        StopCount,
        Stops,
        TourPath, // @todo get from tour?
        TourObj,
    };

    explicit AvailableToursModel(QObject * parent = nullptr);
    ~AvailableToursModel();

public: // QAbstractListModel
	int rowCount(const QModelIndex & parent = {}) const override;
	QVariant data(const QModelIndex & index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public:
	void Update();

private:
	void OnNetworkReplyFinished(QNetworkReply * reply);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};