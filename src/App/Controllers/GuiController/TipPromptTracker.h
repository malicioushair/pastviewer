#pragma once

#include <QDateTime>

class QSettings;

namespace PastViewer {
class TipPromptTracker
{
public:
	TipPromptTracker();

	void RecordSuccessfulRecreation();
	bool ShouldShowPrompt(const QDateTime & nowUtc) const;
	void MarkPromptShown(const QDateTime & nowUtc);
	void MarkPromptDismissed(const QDateTime & nowUtc);
	void DisableAutomaticPrompts();

	int SuccessfulRecreationSessionCount() const;
	int AutomaticPromptCount() const;
	bool AutomaticPromptsDisabled() const;

private:
	quint64 m_sessionId = 0;
};
} // namespace PastViewer
