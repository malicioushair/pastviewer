#pragma once

#include <memory>

#include <QObject>
#include <QQmlEngine>

#include "App/Models/I18nModel.h"
#include "App/Utils/NonCopyMovable.h"

class I18nController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(I18nModel * languageModel READ GetLanguageModel CONSTANT)
	Q_PROPERTY(QString currentLanguage READ GetCurrentLanguage WRITE SetCurrentLanguage NOTIFY CurrentLanguageChanged)

public:
	I18nController(QQmlEngine & engine, QObject * parent = nullptr);
	~I18nController();

	NON_COPY_MOVABLE(I18nController);

signals:
	void CurrentLanguageChanged();

public:
	Q_INVOKABLE void SetCurrentLanguage(const QString & langCode);
	Q_INVOKABLE QString GetCurrentLanguage();
	Q_INVOKABLE int GetIndexOf(const QString & code) const;

	I18nModel * GetLanguageModel();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};