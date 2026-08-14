#pragma once

#include <QString>

namespace PlatformDependentLogic {

void InitializeNotifications();
void ShowPhotoProximityNotification(QString title, QString body, int photoId);
bool SaveScreenshotToGallery(QString filePath);
bool ShareImage(QString filePath);
}
