#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QList>
#include <QObject>
#include <QUrl>

class PositionSourceAdapter;
class QQmlEngine;
class QJSEngine;
class QNetworkReply;

namespace Tours {
struct Tour;
}

class TourController
	: public QObject
{
	Q_OBJECT

public:
	TourController(const PositionSourceAdapter & positionSource, QObject * parent = nullptr);
	~TourController();

	Q_PROPERTY(QList<QGeoCoordinate> tourPath READ GetTourPath NOTIFY TourPathChanged)
	Q_PROPERTY(QString distance READ GetDistance NOTIFY DistanceChanged)
    Q_PROPERTY(QString currentStops READ GetCurrentStops NOTIFY CurrentStopsChanged)

signals:
    void TourPathChanged();
    void DistanceChanged();
    void TourPublished(int row, bool success, const QString & errorMessage);
    void ImageUploadFailed(const QString & errorMessage);
    void UploadTour();
    void CurrentStopsChanged();

    // signals into qml
    void assetUploadFinished();

public:
	Q_INVOKABLE void StartRecording();
	Q_INVOKABLE void PauseRecording();
    Q_INVOKABLE void ResumeRecording();
    Q_INVOKABLE void StopRecording();
    Q_INVOKABLE void CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile);
    Q_INVOKABLE void CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile);
    Q_INVOKABLE QAbstractListModel * GetDraftsModel() const;
    Q_INVOKABLE QAbstractListModel * GetAvailableToursModel() const;
    Q_INVOKABLE void UploadAsset(const QUrl & assetFile);

private:
	QList<QGeoCoordinate> GetTourPath() const;
    QString GetDistance() const;
    QString GetCurrentStops() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
