#include "App/Controllers/GuiController/platform/Logic.h"

#include <QFileInfo>
#include <QStandardPaths>

namespace PlatformDependentLogic {

bool SaveScreenshotToGallery(const QString & filePath)
{
	return true;
}

} // namespace PlatformDependentLogic