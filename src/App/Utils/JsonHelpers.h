#pragma once

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "App/Tours/Tours.h"

#include "glog/logging.h"

namespace JsonHelpers {

inline bool SaveJson(const QJsonDocument & json, const QString & prefix)
{
	QDir().mkpath(Tours::GetDraftsLocation());
	QFile file(Tours::GetDraftFileLocation(prefix));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	const auto data = json.toJson(QJsonDocument::Compact);
	return file.write(data) == data.size();
}

inline QJsonObject ReadJson(const QString & path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return {};

	const auto data = file.readAll();
	QJsonParseError error;
	const auto document = QJsonDocument::fromJson(data, &error);

	if (error.error != QJsonParseError::NoError)
	{
		LOG(WARNING) << "Error when parsing json: " << error.errorString().data();
		return {};
	}

	if (!document.isObject())
	{
		LOG(WARNING) << "Json is not an object";
		return {};
	}

	return document.object();
}

}
