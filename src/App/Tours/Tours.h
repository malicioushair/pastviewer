#pragma once

#include <iterator>
#include <optional>
#include <ranges>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "App/Utils/JsonHelpers.h"

namespace Tours {

inline QString GetDraftsLocation()
{
	return QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "draft_tours");
}

inline QString GetDraftFileLocation(const QString & prefix)
{
	return QString("%1/%2_draft.json").arg(GetDraftsLocation(), prefix);
}

struct TourStop
{
	int64_t id {};
	QString title {};
	QString description {};
    QList<QUrl> localImagePaths {};
    QStringList imageFileHashes {};
    QUrl localAudioPath {};
    QString audioFileHash {};
    QGeoCoordinate coords {};
};

struct Tour
{
	int64_t id {};
	QString title {};
	QString description {};
    double distance {};
    QUrl localImagePath {};
    QString imageFileHash {};
    QList<QGeoCoordinate> tourPath {};
    QList<TourStop> stops {};
    QString lang {};
};

inline std::optional<QString> ChosenFileContentHash(const QUrl & fileUrl)
{
	if (fileUrl.isEmpty())
		return QString();

	const auto basename = fileUrl.isLocalFile() ? QFileInfo(fileUrl.toLocalFile()).fileName() : fileUrl.fileName();
	QFile file(fileUrl.toLocalFile());
	if (!file.open(QIODevice::ReadOnly))
		return std::nullopt;

	QCryptographicHash hasher(QCryptographicHash::Sha256);
	if (!hasher.addData(&file))
		return std::nullopt;

	return hasher.result().toHex();
}

inline std::optional<QJsonObject> StopToJson(const TourStop & stop)
{
    QJsonArray imageFileHashes;
    for (const auto & imagePath : stop.localImagePaths)
    {
        const auto imageFileHash = ChosenFileContentHash(imagePath);
        if (!imageFileHash)
            return std::nullopt;
        imageFileHashes.append(*imageFileHash);
    }

    const auto audioFileHash = ChosenFileContentHash(stop.localAudioPath);
    if (!audioFileHash)
        return std::nullopt;

    const auto coords = QJsonObject {
        { "latitude",  stop.coords.latitude()  },
        { "longitude", stop.coords.longitude() },
    };
    return QJsonObject {
        { "title",           stop.title            },
        { "description",     stop.description      },
        { "imageFileHashes", imageFileHashes       },
        { "audioFileHash",   audioFileHash.value() },
        { "coords",          coords                },
    };
}

inline QJsonObject TourToJson(const Tour & tour)
{
	const auto imageFileHash = ChosenFileContentHash(tour.localImagePath);
	if (!imageFileHash)
		return {};

	QJsonArray stops;
	for (const auto & stop : tour.stops)
	{
		const auto stopJson = StopToJson(stop);
		if (!stopJson)
			return {};
		stops.append(*stopJson);
	}

    QJsonArray pathCoords;
    for (const auto & point : tour.tourPath)
        pathCoords.append(QJsonObject {
            { "latitude",  point.latitude()  },
            { "longitude", point.longitude() },
        });

    return {
        { "schemaVersion", 1                                        },
        { "title",         tour.title                               },
        { "description",   tour.description                         },
        { "lang",          tour.lang                                },
        { "imageFileHash", *imageFileHash                           },
        { "distance",      tour.distance                            },
        { "stops",         stops                                    },
        { "path",          QJsonObject { { "coords", pathCoords } } },
        { "isDraft",       true                                     }, //@todo: needed on the server?
    };
}

inline bool SaveDraft(const Tours::Tour & tour)
{
    QJsonArray stops;
    for (const auto & stop : tour.stops)
    {
        QJsonArray localImagePaths;
        for (const auto & path : stop.localImagePaths)
            localImagePaths.append(path.toString());
        QJsonArray imageFileHashes;
        for (const auto & hash : stop.imageFileHashes)
            imageFileHashes.append(hash);

        QJsonObject stopObj;
        stopObj["id"] = static_cast<qint64>(stop.id);
        stopObj["title"] = stop.title;
        stopObj["description"] = stop.description;
        stopObj["localImagePaths"] = localImagePaths;
        stopObj["imageFileHashes"] = imageFileHashes;
        stopObj["localAudioPath"] = stop.localAudioPath.toString();
        stopObj["audioFileHash"] = stop.audioFileHash;
        stopObj["coords"] = QString("%1, %2").arg(stop.coords.latitude()).arg(stop.coords.longitude());
        stops.append(stopObj);
    }

    QJsonArray tourPath;
    std::ranges::transform(tour.tourPath, std::back_inserter(tourPath), [](decltype(tour.tourPath)::const_reference item) {
        return QString("%1, %2").arg(item.latitude()).arg(item.longitude());
    });

    QJsonObject tourObj;
    tourObj["schemaVersion"] = 1;
    tourObj["id"] = static_cast<qint64>(tour.id);
    tourObj["title"] = tour.title;
    tourObj["description"] = tour.description;
    tourObj["distance"] = tour.distance;
    tourObj["localImagePath"] = tour.localImagePath.toString();
    tourObj["imageFileHash"] = tour.imageFileHash;
    tourObj["path"] = tourPath;
    tourObj["stops"] = stops;
    tourObj["lang"] = tour.lang;

    QJsonDocument doc(tourObj);
    return JsonHelpers::SaveJson(doc, Tours::GetDraftFileLocation(tour.title));
}
}
