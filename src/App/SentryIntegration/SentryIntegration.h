#pragma once

#include <QString>
#include <string>

namespace SentryIntegration {

// Platform abstraction interface
class ISentry
{
public:
	virtual ~ISentry() = default;
	virtual bool Initialize(const QString & release) = 0;
	virtual void Shutdown() = 0;
	virtual void AddBreadcrumb(const std::string & message, const std::string & level) = 0;
	virtual void CaptureException(const QString & message, const QString & type) = 0;
	virtual void Flush() = 0;
};

// Get platform-specific implementation
ISentry & GetPlatform();

// Initialize Sentry SDK and install integrations (breadcrumbs + exception handler)
// Returns true if initialization was successful
bool InitSentry(const QString & release);

// Install glog sink to send logs as Sentry breadcrumbs
void InstallBreadcrumbSink();

// Install global exception handler to capture uncaught exceptions to Sentry
void InstallExceptionHandler();

} // namespace SentryIntegration
