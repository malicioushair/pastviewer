#pragma once

#include <functional>

#include <QString>

namespace PlatformDependentLogic {

using NotificationTappedHandler = std::function<void(int)>;

void InitializeNotifications(NotificationTappedHandler notificationTappedHandler);
void SetBackgroundLocationTrackingEnabled(bool enabled);
void ShowPhotoProximityNotification(QString title, QString body, int photoId);
bool SaveScreenshotToGallery(QString filePath);
bool ShareImage(QString filePath);
}
