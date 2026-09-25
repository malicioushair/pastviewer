#pragma once

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

struct TourStop
{
	int64_t id {};
	QString title {};
	QString description {};
	QUrl imageFile {};
	QUrl audioFile {};
	QGeoCoordinate coords {};
};

struct Tour
{
	int64_t id {};
	QString title {};
	QString description {};
	QUrl imageFile {};
	QList<TourStop> stops {};
};

inline QJsonObject StopToJson(const TourStop & stop)
{
	const auto coords = QJsonObject {
		{ "latitude",  stop.coords.latitude()  },
		{ "longitude", stop.coords.longitude() },
	};
	return {
		{ "id",          static_cast<qint64>(stop.id) },
		{ "title",       stop.title                   },
		{ "description", stop.description             },
		{ "imageFile",   stop.imageFile.toString()    },
		{ "audioFile",   stop.audioFile.toString()    },
		{ "coords",      coords                       },
	};
}

inline QJsonObject TourToJson(const Tour & tour, bool forPublish = false)
{
	QJsonArray stops;
	for (const auto & stop : tour.stops)
		stops.append(StopToJson(stop));
	return {
		{ "schemaVersion", 1                            },
		{ "id",            static_cast<qint64>(tour.id) },
		{ "title",         tour.title                   },
		{ "description",   tour.description             },
		{ "imageFile",     tour.imageFile.toString()    },
		{ "isDraft",       !forPublish                  },
		{ "stops",         stops                        },
	};
}
}
