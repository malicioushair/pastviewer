#pragma once

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
};

struct Tour
{
	int64_t id {};
	QString title {};
	QString descrption {};
	QUrl imageFile {};
	QList<TourStop> stops {};
	bool isDraft {};
};

}
