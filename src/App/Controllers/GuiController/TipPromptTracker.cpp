#include "TipPromptTracker.h"

#include <QSettings>

#include <QtCore/qsettings.h>
#include <algorithm>

using namespace PastViewer;

namespace {

constexpr int REQUIRED_SUCCESSFUL_SESSIONS = 2;
constexpr int MAX_AUTOMATIC_PROMPTS = 2;
constexpr int REMINDER_COOLDOWN_DAYS = 30;

constexpr auto SESSION_SERIAL_KEY = "Tips/SessionSerial";
constexpr auto LAST_SUCCESSFUL_SESSION_KEY = "Tips/LastSuccessfulRecreationSession";
constexpr auto SUCCESSFUL_SESSION_COUNT_KEY = "Tips/SuccessfulRecreationSessionCount";
constexpr auto AUTOMATIC_PROMPT_COUNT_KEY = "Tips/AutomaticPromptCount";
constexpr auto LAST_PROMPT_SHOWN_KEY = "Tips/LastPromptShownUtc";
constexpr auto LAST_PROMPT_DISMISSED_KEY = "Tips/LastPromptDismissedUtc";
constexpr auto AUTOMATIC_PROMPTS_DISABLED_KEY = "Tips/AutomaticPromptsDisabled";

QDateTime ReadUtcDateTime(const QSettings & settings, const char * key)
{
	return settings.value(key).toDateTime().toUTC();
}

} // namespace

TipPromptTracker::TipPromptTracker()
{
	QSettings settings;
	m_sessionId = settings.value(SESSION_SERIAL_KEY, 0).toULongLong() + 1;
	settings.setValue(SESSION_SERIAL_KEY, m_sessionId);
}

void TipPromptTracker::RecordSuccessfulRecreation()
{
	QSettings settings;
	if (settings.value(LAST_SUCCESSFUL_SESSION_KEY, 0).toULongLong() == m_sessionId)
		return;

	settings.setValue(LAST_SUCCESSFUL_SESSION_KEY, m_sessionId);
	const auto sessionCount = std::min(
		SuccessfulRecreationSessionCount() + 1,
		REQUIRED_SUCCESSFUL_SESSIONS);
	settings.setValue(SUCCESSFUL_SESSION_COUNT_KEY, sessionCount);
}

bool TipPromptTracker::ShouldShowPrompt(const QDateTime & nowUtc) const
{
	if (AutomaticPromptsDisabled()
		|| SuccessfulRecreationSessionCount() < REQUIRED_SUCCESSFUL_SESSIONS)
		return false;

	const auto promptCount = AutomaticPromptCount();
	if (promptCount == 0)
		return true;
	if (promptCount >= MAX_AUTOMATIC_PROMPTS)
		return false;

	QSettings settings;
	auto cooldownStart = ReadUtcDateTime(settings, LAST_PROMPT_DISMISSED_KEY);
	if (!cooldownStart.isValid())
		cooldownStart = ReadUtcDateTime(settings, LAST_PROMPT_SHOWN_KEY);

	return cooldownStart.isValid()
		&& cooldownStart.addDays(REMINDER_COOLDOWN_DAYS) <= nowUtc.toUTC();
}

void TipPromptTracker::MarkPromptShown(const QDateTime & nowUtc)
{
	const auto promptCount = std::min(
		AutomaticPromptCount() + 1,
		MAX_AUTOMATIC_PROMPTS);
	QSettings settings;
	settings.setValue(AUTOMATIC_PROMPT_COUNT_KEY, promptCount);
	settings.setValue(LAST_PROMPT_SHOWN_KEY, nowUtc.toUTC());
}

void TipPromptTracker::MarkPromptDismissed(const QDateTime & nowUtc)
{
	QSettings settings;
	settings.setValue(LAST_PROMPT_DISMISSED_KEY, nowUtc.toUTC());
}

void TipPromptTracker::DisableAutomaticPrompts()
{
	QSettings settings;
	settings.setValue(AUTOMATIC_PROMPTS_DISABLED_KEY, true);
}

int TipPromptTracker::SuccessfulRecreationSessionCount() const
{
	QSettings settings;
	return settings.value(SUCCESSFUL_SESSION_COUNT_KEY, 0).toInt();
}

int TipPromptTracker::AutomaticPromptCount() const
{
	QSettings settings;
	return settings.value(AUTOMATIC_PROMPT_COUNT_KEY, 0).toInt();
}

bool TipPromptTracker::AutomaticPromptsDisabled() const
{
	QSettings settings;
	return settings.value(AUTOMATIC_PROMPTS_DISABLED_KEY, false).toBool();
}
