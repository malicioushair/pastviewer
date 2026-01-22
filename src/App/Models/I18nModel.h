#pragma once

#include <QAbstractListModel>
#include <QStringList>

class I18nModel : public QAbstractListModel
{
	Q_OBJECT

public:
	enum LanguageRoles
	{
		CodeRole = Qt::UserRole + 1,
		NameRole,
	};

	struct LanguageItem
	{
		QString code;
		QString name;
	};

	explicit I18nModel(QObject * parent = nullptr);

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	using LanguageItems = std::array<LanguageItem, 12>;
	LanguageItems GetAllLanguages();
};
