#pragma once

#include <memory>

#include <QObject>
#include <QQmlEngine>

#include "App/Models/I18nModel.h"
#include "App/Utils/NonCopyMovable.h"

class I18nController : public QObject
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON

	Q_PROPERTY(I18nModel * languageModel READ GetLanguageModel CONSTANT)
	Q_PROPERTY(QString currentLanguage READ GetCurrentLanguage WRITE SetCurrentLanguage NOTIFY CurrentLanguageChanged)

public:
	static I18nController * create(QQmlEngine * qmlEngine, QJSEngine * jsEngine);
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
	I18nController(QQmlEngine & engine, QObject * parent = nullptr);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};