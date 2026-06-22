#include "App/Controllers/GuiController/platform/Logic.h"

#include <QFileInfo>

#include "glog/logging.h"

#import <Photos/Photos.h>
#import <UIKit/UIKit.h>

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

bool SaveScreenshotToGallery(const QString & filePath)
{
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists() || fileInfo.size() <= 0)
	{
		LOG(ERROR) << "Image file does not exist or is empty: " << filePath.toStdString();
		return false;
	}

	const auto path = filePath;
	[PHPhotoLibrary requestAuthorizationForAccessLevel:PHAccessLevelAddOnly
											   handler:^(PHAuthorizationStatus status) {
												   if (status != PHAuthorizationStatusAuthorized && status != PHAuthorizationStatusLimited)
												   {
													   NSLog(@"Photos add permission denied or restricted: %ld", (long)status);
													   return;
												   }

												   NSString * nsPath = path.toNSString();
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

bool ShareImage(const QString & filePath)
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
		UIActivityViewController * activityController = [[UIActivityViewController alloc] initWithActivityItems:@[ url ] applicationActivities:nil];

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

} // namespace PlatformDependentLogic
