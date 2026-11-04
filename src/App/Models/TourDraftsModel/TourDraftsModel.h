#pragma once

#include <QAbstractListModel>

#include "App/Tours/Tours.h"
#include "App/Utils/NonCopyMovable.h"
#include "PositionSourceAdapter.h"

#include <memory>
#include <functional>

class PositionSourceAdapter;
class QNetworkAccessManager;

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

        // Upload progress (appended to preserve existing role values)
        Uploading,
    };

    // An injected manager must outlive the model; otherwise the model owns one.
    TourDraftsModel(const PositionSourceAdapter & positionSource, QObject * parent = nullptr,
        QNetworkAccessManager * networkManager = nullptr);
    ~TourDraftsModel();

signals:
    void CurrentDistanceChanged();
    void CurrentTourPathChanged();
    void AssetsUploadFinished();
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
    QString BackupDraft(qlonglong id, const QUrl & destination) const;
    QString UpdateDraftDetails(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile);
    bool SelectDraft(qlonglong id);
    QVariantMap GetDraftStop(qlonglong id) const;
    QString UpdateDraftStop(qlonglong id, const QString & title, const QString & description, const QList<QUrl> & imageFiles, const QUrl & audioFile);
    void UpdateTourPath(const QGeoCoordinate & coords);
    void AddPoint(const QGeoCoordinate & coordinate);
    void RemoveLastPoint();
    QList<QGeoCoordinate> GetTourPath() const;
    QList<QGeoCoordinate> GetCurrentStopCoordinates() const;
    QString CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile, const QString & tourLang);
    void CreateTourStop(const QString & title, const QString & description, const QList<QUrl> & imageFiles, const QUrl & audioFile, const QGeoCoordinate & coordinate = QGeoCoordinate());
    QString GetCurrentStops() const;
    void UploadAssets(const QList<QUrl> & assetFile);

    void StartRecording();
    void PauseRecording();
    void ResumeRecording();
    void StopRecording();

    QString GetDistance();

private:
    using UploadCompletion = std::function<void(const QString &)>;
    struct AssetUpload;
    struct PublishOperation;
    bool StartPublishing(const Tours::Tour & tour);
    void SubmitTour(const std::shared_ptr<PublishOperation> & operation);
    void FinishPublishing(qlonglong id, const QString & error);
    void NotifyUploadingChanged(qlonglong id);
    void StartAssetUploads(const QList<QUrl> & files, UploadCompletion completion,
        const QHash<QUrl, QString> & expectedHashes = {});
    void UploadNextAsset(const std::shared_ptr<AssetUpload> & upload);
    void DeleteTourItem(const QModelIndex & index);
    void EmitCurrentTourPathEdited();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
