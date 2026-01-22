#include "App/Models/I18nModel.h"

#include <QCoreApplication>
#include <QLocale>

namespace {

I18nModel::LanguageItems languages {
	{
     { "zh_CN", "Chinese (Simplified" },
     { "en", "English" },
     { "fr", "French" },
     { "de", "German" },
     { "it", "Italian" },
     { "ja", "Japanese" },
     { "ko", "Korean" },
     { "pt", "Portuguese" },
     { "ru", "Russian" },
     { "sr", "Serbian" },
     { "es", "Spanish" },
	 }
};
}

I18nModel::I18nModel(QObject * parent)
	: QAbstractListModel(parent)
{
}

int I18nModel::rowCount(const QModelIndex &) const
{
	return languages.size();
}

QVariant I18nModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || index.row() >= languages.size())
		return QVariant();

	const auto item = languages.at(index.row());
	switch (role)
	{
		case CodeRole:
			return item.code;
		case NameRole:
			return item.name;
		default:
			return assert(false && "unknown role in LanguageRoles"), QVariant();
	}
}

QHash<int, QByteArray> I18nModel::roleNames() const
{
	QHash<int, QByteArray> roles;
#define ROLENAME(NAME) roles[NAME] = #NAME
	ROLENAME(CodeRole);
	ROLENAME(NameRole);
#undef ROLENAME
	return roles;
}

I18nModel::LanguageItems I18nModel::GetAllLanguages()
{
	return languages;
}
