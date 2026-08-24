#pragma once

#include <functional>

#include <QString>
#include <QStringList>
#include <QVector>

namespace PlatformDependentLogic {

using NotificationTappedHandler = std::function<void(int)>;

struct TipProduct
{
	QString id;
	QString title;
	QString displayPrice;
};

enum class TipPurchaseResult
{
	Succeeded,
	Cancelled,
	Pending,
	Failed
};

using TipProductsHandler = std::function<void(QVector<TipProduct>, QString)>;
using TipPurchaseHandler = std::function<void(TipPurchaseResult, QString)>;

void InitializeNotifications(NotificationTappedHandler notificationTappedHandler);
void SetBackgroundLocationTrackingEnabled(bool enabled);
void ShowPhotoProximityNotification(QString title, QString body, int photoId);
bool SaveScreenshotToGallery(QString filePath);
bool ShareImage(QString filePath);
bool SupportsNativeTipPurchases();
void InitializeTipPurchases(QStringList productIds);
void LoadTipProducts(QStringList productIds, TipProductsHandler handler);
void PurchaseTip(QString productId, TipPurchaseHandler handler);
}
