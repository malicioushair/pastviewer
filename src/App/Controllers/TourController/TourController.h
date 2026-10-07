#pragma once

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qtmetamacros.h>
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

signals:
	void TourPathChanged();
	void DistanceChanged();
	void TourPublished(int row, bool success, const QString & errorMessage);
	void ImageUploadFailed(const QString & errorMessage);
	void UploadTour();

	void assetUploadFinished();

public:
	Q_INVOKABLE void StartRecording();
	Q_INVOKABLE void PauseRecording();
	Q_INVOKABLE void UnpauseRecording();
	Q_INVOKABLE void StopRecording();
	Q_INVOKABLE void CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile);
	Q_INVOKABLE void CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile);
	Q_INVOKABLE QAbstractListModel * GetDraftsModel() const;
	Q_INVOKABLE void UpdateModel();
	Q_INVOKABLE void PublishTour(int row);
	Q_INVOKABLE void UploadAsset(const QUrl & assetFile);
	Q_INVOKABLE QAbstractListModel * GetAvailableToursModel() const;

private:
	QList<QGeoCoordinate> GetTourPath() const;
	QString GetDistance() const;
	int64_t NextTourId() const;
	bool SaveDraft(const Tours::Tour & tour);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
