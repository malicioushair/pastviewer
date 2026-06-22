#include "App/Controllers/GuiController/platform/Logic.h"

#include <QFileInfo>
#include <QStandardPaths>

namespace PlatformDependentLogic {

bool SaveScreenshotToGallery([[maybe_unused]] const QString & filePath)
{
	return true;
}

bool ShareImage([[maybe_unused]] const QString & filePath)
{
	return true;
}

} // namespace PlatformDependentLogic