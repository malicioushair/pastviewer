#include "App/Controllers/I18nController/I18nController.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTranslator>

#include <glog/logging.h>

namespace {

constexpr auto LANGUANGE = "language";
constexpr auto QM_PATH_TEMPLATE = ":/i18n/%1_%2.qm";

bool LoadTranslator(QTranslator & translator, const QString & languageCode)
{
	const auto qmPath = QString(QM_PATH_TEMPLATE).arg(QCoreApplication::applicationName(), languageCode);
	if (!translator.load(qmPath))
	{
		LOG(WARNING) << "Cannot load language from path: " << qmPath.toStdString();
		return false;
	}
	return true;
}

}

struct I18nController::Impl
{
	QQmlEngine & engine;
	QString currentLanguage;
	QTranslator runtimeTranslator;
	I18nModel languageModel;
};

I18nController::I18nController(QQmlEngine & engine, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(engine))
{
	const QSettings settings;
	const auto currentLanguage = settings.value(LANGUANGE).toString();
	LoadTranslator(m_impl->runtimeTranslator, currentLanguage);

	QCoreApplication::installTranslator(&m_impl->runtimeTranslator);
	m_impl->currentLanguage = currentLanguage;
	m_impl->engine.retranslate();
}

I18nController::~I18nController() = default;

void I18nController::SetCurrentLanguage(const QString & languageCode)
{
	if (m_impl->currentLanguage == languageCode)
		return;

	QCoreApplication::removeTranslator(&m_impl->runtimeTranslator);
	if (!LoadTranslator(m_impl->runtimeTranslator, languageCode))
		return;

	QCoreApplication::installTranslator(&m_impl->runtimeTranslator);

	m_impl->currentLanguage = languageCode;
	m_impl->engine.retranslate();

	QSettings settings;
	settings.setValue(LANGUANGE, languageCode);

	emit CurrentLanguageChanged();
}

QString I18nController::GetCurrentLanguage()
{
	return m_impl->currentLanguage;
}

int I18nController::GetIndexOf(const QString & code) const
{
	const auto languages = m_impl->languageModel.GetAllLanguages();
	const auto it = std::ranges::find_if(languages, [&](decltype(languages)::const_reference item) { return item.code == code; });
	if (it == languages.cend())
		return -1; // not found

	return it - languages.cbegin();
}

I18nModel * I18nController::GetLanguageModel()
{
	return &m_impl->languageModel;
}
