#pragma once

#include <QAbstractListModel>

#include "App/Tours/Tours.h"
#include "App/Utils/NonCopyMovable.h"
#include "PositionSourceAdapter.h"

#include <memory>

class PositionSourceAdapter;

class TourDraftsModel
	: public QAbstractListModel
{
public:
    Q_OBJECT
    NON_COPY_MOVABLE(TourDraftsModel)

public:
    enum Roles
    {
        // Getters
        Id = Qt::UserRole + 1,
        Title,
        Description,
        Distance,

        // Setters
        AddStop,
        Publish,
        Delete,
    };

    TourDraftsModel(const PositionSourceAdapter & positionSource, QObject * parent = nullptr);
    ~TourDraftsModel();

signals:
    void CurrentDistanceChanged();
    void CurrentTourPathChanged();
    void AssetUploadFinished();
    void ImageUploadFailed(const QString & errorMessage);
    void StopsUpdated();

public: // QAbstractListModel
    int rowCount(const QModelIndex & parent = {}) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

public:
	void Update();
	std::vector<Tours::Tour> GetTourDrafts() const;
	void AddTour(const Tours::Tour & tour);
    QVariantList GetPlacedStopMarkers() const;
    QVariantMap GetDraft(qlonglong id) const;
    QString UpdateDraftDetails(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile);
    bool SelectDraft(qlonglong id);
    QVariantMap GetDraftStop(qlonglong id) const;
    QString UpdateDraftStop(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile);
    void UpdateTourPath(const QGeoCoordinate & coords);
    void AddPoint(const QGeoCoordinate & coordinate);
    void RemoveLastPoint();
    QList<QGeoCoordinate> GetTourPath() const;
    QList<QGeoCoordinate> GetCurrentStopCoordinates() const;
    void CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile);
    void CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile, const QGeoCoordinate & coordinate = QGeoCoordinate());
    QString GetCurrentStops() const;
    void UploadAsset(const QUrl & assetFile);

    void StartRecording();
    void PauseRecording();
    void ResumeRecording();
    void StopRecording();

    QString GetDistance();

private:
    void DeleteTourItem(const QModelIndex & index);
    void EmitCurrentTourPathEdited();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
