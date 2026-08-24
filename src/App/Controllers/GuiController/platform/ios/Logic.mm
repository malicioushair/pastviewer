#include "App/Controllers/GuiController/platform/Logic.h"

#include <mutex>
#include <utility>

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include "glog/logging.h"

#include "App/Controllers/GuiController/platform/ios/StoreKitBridge.h"

#import <Photos/Photos.h>
#import <UIKit/UIKit.h>
#import <UserNotifications/UserNotifications.h>

static NSString * const PHOTO_ID_KEY = @"photoId";

namespace {
PlatformDependentLogic::NotificationTappedHandler notificationTappedHandler;
std::mutex tipCallbackMutex;
PlatformDependentLogic::TipProductsHandler tipProductsHandler;
PlatformDependentLogic::TipPurchaseHandler tipPurchaseHandler;

template<typename Callback, typename... Args>
void DispatchToQt(Callback callback, Args... args)
{
	if (!callback)
		return;

	auto invoke = [callback = std::move(callback), ... args = std::move(args)]() mutable {
		callback(std::move(args)...);
	};
	if (auto * application = QCoreApplication::instance())
		QMetaObject::invokeMethod(application, std::move(invoke), Qt::QueuedConnection);
	else
		invoke();
}

void StoreKitProductsLoaded(const char * productsJson, const char * errorMessage)
{
	PlatformDependentLogic::TipProductsHandler handler;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		handler = std::move(tipProductsHandler);
	}

	QVector<PlatformDependentLogic::TipProduct> products;
	QString error = errorMessage ? QString::fromUtf8(errorMessage) : QString {};
	if (productsJson)
	{
		QJsonParseError parseError;
		const auto document = QJsonDocument::fromJson(QByteArray(productsJson), &parseError);
		if (parseError.error != QJsonParseError::NoError || !document.isArray())
			error = QStringLiteral("Could not parse StoreKit product data: %1").arg(parseError.errorString());
		else
		{
			for (const auto & value : document.array())
			{
				const auto object = value.toObject();
				const auto id = object.value(QStringLiteral("id")).toString();
				const auto title = object.value(QStringLiteral("title")).toString();
				const auto displayPrice = object.value(QStringLiteral("displayPrice")).toString();
				if (!id.isEmpty() && !displayPrice.isEmpty())
					products.push_back({ id, title, displayPrice });
			}
		}
	}

	DispatchToQt(std::move(handler), std::move(products), std::move(error));
}

void StoreKitPurchaseFinished(int result, const char * errorMessage)
{
	PlatformDependentLogic::TipPurchaseHandler handler;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		handler = std::move(tipPurchaseHandler);
	}

	auto purchaseResult = PlatformDependentLogic::TipPurchaseResult::Failed;
	switch (result)
	{
	case 0:
		purchaseResult = PlatformDependentLogic::TipPurchaseResult::Succeeded;
		break;
	case 1:
		purchaseResult = PlatformDependentLogic::TipPurchaseResult::Cancelled;
		break;
	case 2:
		purchaseResult = PlatformDependentLogic::TipPurchaseResult::Pending;
		break;
	default:
		break;
	}

	DispatchToQt(
		std::move(handler),
		purchaseResult,
		errorMessage ? QString::fromUtf8(errorMessage) : QString {});
}
}

@interface PastViewerNotificationDelegate : NSObject<UNUserNotificationCenterDelegate>
@end

@implementation PastViewerNotificationDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
	   willPresentNotification:(UNNotification *)notification
		 withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
	completionHandler(UNNotificationPresentationOptionNone);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
	didReceiveNotificationResponse:(UNNotificationResponse *)response
			 withCompletionHandler:(void (^)(void))completionHandler
{
	NSNumber * photoId = response.notification.request.content.userInfo[PHOTO_ID_KEY];
	if ([photoId isKindOfClass:[NSNumber class]] && notificationTappedHandler)
		notificationTappedHandler(photoId.intValue);

	completionHandler();
}

@end

namespace PlatformDependentLogic {

namespace {

UIViewController * RootViewController()
{
	UIApplication * application = [UIApplication sharedApplication];
	if (@available(iOS 13.0, *))
	{
		for (UIScene * scene in application.connectedScenes)
		{
			if (scene.activationState != UISceneActivationStateForegroundActive)
				continue;

			if (![scene isKindOfClass:[UIWindowScene class]])
				continue;

			for (UIWindow * window in ((UIWindowScene *)scene).windows)
			{
				if (window.isKeyWindow)
					return window.rootViewController;
			}
		}
	}

	return application.keyWindow.rootViewController;
}

} // namespace

void InitializeNotifications(NotificationTappedHandler handler)
{
	notificationTappedHandler = std::move(handler);

	static PastViewerNotificationDelegate * notificationDelegate = [[PastViewerNotificationDelegate alloc] init];
	UNUserNotificationCenter * center = [UNUserNotificationCenter currentNotificationCenter];
	center.delegate = notificationDelegate;
	[center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
						  completionHandler:^(BOOL granted, NSError * error) {
							  if (error)
								  NSLog(@"Failed to request notification authorization: %@", error);
							  else if (!granted)
								  NSLog(@"Notification authorization was not granted");
						  }];
}

void SetBackgroundLocationTrackingEnabled(bool)
{
}

void ShowPhotoProximityNotification(const QString title, const QString body, int photoId)
{
	UNUserNotificationCenter * center = [UNUserNotificationCenter currentNotificationCenter];
	[center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings * settings) {
		if (settings.authorizationStatus != UNAuthorizationStatusAuthorized
			&& settings.authorizationStatus != UNAuthorizationStatusProvisional)
			return;

		UNMutableNotificationContent * content = [[UNMutableNotificationContent alloc] init];
		content.title = title.toNSString();
		content.body = body.toNSString();
		content.sound = [UNNotificationSound defaultSound];
		content.userInfo = @{PHOTO_ID_KEY: @(photoId)};

		NSString * identifier = [NSString stringWithFormat:@"photo-area-%d", photoId];
		UNNotificationRequest * request = [UNNotificationRequest requestWithIdentifier:identifier
																			   content:content
																			   trigger:nil];
		[center addNotificationRequest:request withCompletionHandler:^(NSError * error) {
			if (error)
				NSLog(@"Failed to schedule photo proximity notification: %@", error);
		}];
	}];
}

bool SaveScreenshotToGallery(const QString filePath)
{
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists() || fileInfo.size() <= 0)
	{
		LOG(ERROR) << "Image file does not exist or is empty: " << filePath.toStdString();
		return false;
	}

	[PHPhotoLibrary requestAuthorizationForAccessLevel:PHAccessLevelAddOnly
											   handler:^(PHAuthorizationStatus status) {
												   if (status != PHAuthorizationStatusAuthorized && status != PHAuthorizationStatusLimited)
												   {
													   NSLog(@"Photos add permission denied or restricted: %ld", (long)status);
													   return;
												   }

												   NSString * nsPath = filePath.toNSString();
												   UIImage * image = [UIImage imageWithContentsOfFile:nsPath];
												   if (!image)
												   {
													   NSLog(@"Failed to decode image before saving to Photos: %@", nsPath);
													   return;
												   }

												   [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
													   [PHAssetCreationRequest creationRequestForAssetFromImage:image];
												   } completionHandler:^(BOOL success, NSError * error) {
													   if (!success)
														   NSLog(@"Failed to save to Photos: %@ userInfo=%@", error, error.userInfo);
												   }];
											   }];

	return true;
}

bool ShareImage(const QString filePath)
{
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists())
	{
		LOG(ERROR) << "File does not exist: " << filePath.toStdString();
		return false;
	}

	@autoreleasepool
	{
		UIViewController * rootViewController = RootViewController();
		if (!rootViewController)
		{
			LOG(ERROR) << "Failed to get root view controller";
			return false;
		}

		NSURL * url = [NSURL fileURLWithPath:filePath.toNSString()];
		UIActivityViewController * activityController = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];

		if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad)
		{
			UIPopoverPresentationController * popover = activityController.popoverPresentationController;
			popover.sourceView = rootViewController.view;
			popover.sourceRect = CGRectMake(
				CGRectGetMidX(rootViewController.view.bounds),
				CGRectGetMidY(rootViewController.view.bounds),
				0,
				0);
			popover.permittedArrowDirections = 0;
		}

		[rootViewController presentViewController:activityController animated:YES completion:nil];
	}

	return true;
}

bool SupportsNativeTipPurchases()
{
	return true;
}

void LoadTipProducts(QStringList productIds, TipProductsHandler handler)
{
	bool requestInProgress = false;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		if (tipProductsHandler)
			requestInProgress = true;
		else
			tipProductsHandler = std::move(handler);
	}
	if (requestInProgress)
	{
		handler({}, QStringLiteral("A StoreKit product request is already in progress."));
		return;
	}

	const auto productIdsCsv = productIds.join(QLatin1Char(',')).toUtf8();
	PastViewerStoreKitLoadProducts(productIdsCsv.constData(), StoreKitProductsLoaded);
}

void PurchaseTip(QString productId, TipPurchaseHandler handler)
{
	bool purchaseInProgress = false;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		if (tipPurchaseHandler)
			purchaseInProgress = true;
		else
			tipPurchaseHandler = std::move(handler);
	}
	if (purchaseInProgress)
	{
		handler(TipPurchaseResult::Failed, QStringLiteral("A StoreKit purchase is already in progress."));
		return;
	}

	const auto productIdUtf8 = productId.toUtf8();
	PastViewerStoreKitPurchase(productIdUtf8.constData(), StoreKitPurchaseFinished);
}

} // namespace PlatformDependentLogic
