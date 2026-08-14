#include "App/Controllers/GuiController/platform/Logic.h"

#include <QFileInfo>
#include <QStandardPaths>

namespace PlatformDependentLogic {

void InitializeNotifications(NotificationTappedHandler)
{
}

void ShowPhotoProximityNotification(const QString, const QString, int)
{
}

bool SaveScreenshotToGallery(const QString)
{
	return true;
}

bool ShareImage(const QString)
{
	return true;
}

} // namespace PlatformDependentLogic
