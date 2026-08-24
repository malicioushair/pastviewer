#include "App/Controllers/GuiController/platform/Logic.h"

#include <QFileInfo>
#include <QStandardPaths>

namespace PlatformDependentLogic {

void InitializeNotifications(NotificationTappedHandler)
{
}

void SetBackgroundLocationTrackingEnabled(bool)
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

bool SupportsNativeTipPurchases()
{
	return false;
}

void LoadTipProducts(QStringList, TipProductsHandler handler)
{
	handler({}, QStringLiteral("Native tip purchases are unavailable on this platform."));
}

void PurchaseTip(QString, TipPurchaseHandler handler)
{
	handler(TipPurchaseResult::Failed, QStringLiteral("Native tip purchases are unavailable on this platform."));
}

} // namespace PlatformDependentLogic
