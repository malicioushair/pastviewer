#pragma once

#include <optional>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>
#include <QUrl>

namespace Tours {

inline QString GetDraftsLocation()
{
	return QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "draft_tours");
}

inline QString GetDraftFileLocation(const QString & prefix)
{
	return QString("%1/%2_draft.json").arg(GetDraftsLocation(), prefix);
}

struct TourStop // add path
{
	int64_t id {};
	QString title {};
	QString description {};
	QUrl localImagePath {};
	QString imageFileHash {};
	QUrl localAudioPath {};
	QString audioFileHash {};
	QGeoCoordinate coords {};
};

struct Tour
{
	int64_t id {};
	QString title {};
	QString description {};
	QUrl localImagePath {};
	QString imageFileHash {};
	QList<TourStop> stops {};
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
	const auto imageFileHash = ChosenFileContentHash(stop.localImagePath);
	const auto audioFileHash = ChosenFileContentHash(stop.localAudioPath);
	if (!imageFileHash || !audioFileHash)
		return std::nullopt;

	const auto coords = QJsonObject {
		{ "latitude",  stop.coords.latitude()  },
		{ "longitude", stop.coords.longitude() },
	};
	return QJsonObject {
		{ "id",            static_cast<qint64>(stop.id)   },
		{ "title",         stop.title                     },
		{ "description",   stop.description               },
		{ "imageFile",     stop.localImagePath.toString() },
		{ "imageFileHash", *imageFileHash                 },
		{ "audioFile",     stop.localAudioPath.toString() },
		{ "audioFileHash", *audioFileHash                 },
		{ "coords",        coords                         },
	};
}

inline QJsonObject TourToJson(const Tour & tour, bool forPublish = false)
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
	return {
		{ "schemaVersion", 1							  },
		{ "id",            static_cast<qint64>(tour.id)   },
		{ "title",         tour.title                     },
		{ "description",   tour.description               },
		{ "imageFile",     tour.localImagePath.toString() },
		{ "imageFileHash", *imageFileHash                 },
		{ "isDraft",       !forPublish                    },
		{ "stops",         stops						  },
	};
}
}
