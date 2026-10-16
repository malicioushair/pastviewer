#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QList>
#include <QObject>
#include <QUrl>
#include <QVariant>

#include "App/Tours/DisplayTours.h"

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
    Q_PROPERTY(QString distance READ GetDistance NOTIFY DistanceChanged)             // @TODO creation distance?
    Q_PROPERTY(QString currentStops READ GetCurrentStops NOTIFY CurrentStopsChanged) // @TODO creation stops?
    Q_PROPERTY(QVariantList placedStopMarkers READ GetPlacedStopMarkers NOTIFY CurrentStopsChanged)
    Q_PROPERTY(QVariant currentTour READ GetCurrentTour WRITE SetCurrentTour NOTIFY CurrentTourChanged)
    Q_PROPERTY(QString nextStopTitle READ GetSelectedStopTitle NOTIFY SelectedStopTitleChanged)

signals:
    void TourPathChanged();
    void DistanceChanged();
    void TourPublished(int row, bool success, const QString & errorMessage);
    void ImageUploadFailed(const QString & errorMessage);
    void UploadTour();
    void CurrentStopsChanged();
    void CurrentTourChanged();
    void SelectedStopTitleChanged();

    void selectedStopChanged(const DisplayTourStop & stop);

    // signals into qml
    void
    assetUploadFinished();

public:
	Q_INVOKABLE void StartRecording();
	Q_INVOKABLE void PauseRecording();
    Q_INVOKABLE void ResumeRecording();
    Q_INVOKABLE void StopRecording();
    Q_INVOKABLE void CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile);
    Q_INVOKABLE void CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile, const QGeoCoordinate & coordinate = QGeoCoordinate());
    Q_INVOKABLE void AddPoint(const QGeoCoordinate & coordinate);
    Q_INVOKABLE void RemoveLastPoint();
    Q_INVOKABLE QAbstractListModel * GetDraftsModel() const;
    Q_INVOKABLE QAbstractListModel * GetAvailableToursModel() const;
    Q_INVOKABLE QAbstractListModel * GetTourStopsModel() const;
    Q_INVOKABLE void UploadAsset(const QUrl & assetFile);
    Q_INVOKABLE void HighlightNextStop();
    Q_INVOKABLE void HighlightPrevStop();

private:
	QList<QGeoCoordinate> GetTourPath() const;
    QString GetDistance() const;
    QString GetCurrentStops() const;
    QVariantList GetPlacedStopMarkers() const;
    QVariant GetCurrentTour() const;
    void SetCurrentTour(const QVariant & tour);
    QString GetSelectedStopTitle() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
