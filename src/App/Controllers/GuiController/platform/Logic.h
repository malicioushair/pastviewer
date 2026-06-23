#pragma once

#include <QString>

namespace PlatformDependentLogic {

bool SaveScreenshotToGallery(const QString & filePath);
bool ShareImage(const QString & filePath);

}