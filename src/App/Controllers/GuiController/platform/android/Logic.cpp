#include "App/Controllers/GuiController/platform/Logic.h"

#include <mutex>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QStandardPaths>

#include "glog/logging.h"

namespace PlatformDependentLogic {

namespace {

constexpr auto QT_NATIVE = "org/qtproject/qt/android/QtNative";
constexpr auto ACTIVITY = "activity";
constexpr auto NOTIFICATION_HELPER = "org/qtproject/PastViewer/NotificationHelper";
constexpr auto LOCATION_FOREGROUND_SERVICE = "org/qtproject/PastViewer/LocationForegroundService";
constexpr auto SHARE_HELPER = "org/qtproject/PastViewer/ShareHelper";
constexpr auto GOOGLE_PLAY_BILLING_HELPER = "org/qtproject/PastViewer/GooglePlayBillingHelper";

NotificationTappedHandler notificationTappedHandler;
std::mutex tipCallbackMutex;
TipProductsHandler tipProductsHandler;
TipPurchaseHandler tipPurchaseHandler;

QJniObject Activity()
{
	return QJniObject::callStaticObjectMethod(
		QT_NATIVE,
		ACTIVITY,
		"()Landroid/app/Activity;");
}

QString FromJString(JNIEnv * env, jstring value)
{
	if (!value)
		return {};

	const auto length = env->GetStringLength(value);
	const auto * characters = env->GetStringChars(value, nullptr);
	if (!characters)
		return {};

	const auto result = QString::fromUtf16(
		reinterpret_cast<const char16_t *>(characters),
		length);
	env->ReleaseStringChars(value, characters);
	return result;
}

template <typename Callback, typename... Args>
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
}

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_NotificationHelper_notificationTapped(JNIEnv *, jclass, jint photoId)
{
	if (notificationTappedHandler)
		notificationTappedHandler(static_cast<int>(photoId));
}

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_GooglePlayBillingHelper_productsLoaded(
	JNIEnv * env,
	jclass,
	jstring productsJson,
	jstring errorMessage)
{
	TipProductsHandler handler;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		handler = std::move(tipProductsHandler);
	}

	QVector<TipProduct> products;
	auto error = FromJString(env, errorMessage);
	if (productsJson)
	{
		QJsonParseError parseError;
		const auto document = QJsonDocument::fromJson(FromJString(env, productsJson).toUtf8(), &parseError);
		if (parseError.error != QJsonParseError::NoError || !document.isArray())
			error = QStringLiteral("Could not parse Google Play product data: %1").arg(parseError.errorString());
		else
		{
			for (const auto & value : document.array())
			{
				const auto product = value.toObject();
				products.push_back({
					.id = product.value(QStringLiteral("id")).toString(),
					.title = product.value(QStringLiteral("title")).toString(),
					.displayPrice = product.value(QStringLiteral("displayPrice")).toString(),
				});
			}
		}
	}

	DispatchToQt(std::move(handler), std::move(products), std::move(error));
}

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_GooglePlayBillingHelper_purchaseFinished(
	JNIEnv * env,
	jclass,
	jint result,
	jstring errorMessage)
{
	TipPurchaseHandler handler;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		handler = std::move(tipPurchaseHandler);
	}

	const auto purchaseResult = [result] {
		switch (result)
		{
			case 0:
				return TipPurchaseResult::Succeeded;
			case 1:
				return TipPurchaseResult::Cancelled;
			case 2:
				return TipPurchaseResult::Pending;
			default:
				return TipPurchaseResult::Failed;
		}
	}();
	DispatchToQt(std::move(handler), purchaseResult, FromJString(env, errorMessage));
}

void InitializeNotifications(NotificationTappedHandler handler)
{
	notificationTappedHandler = std::move(handler);

	const auto activity = Activity();
	if (!activity.isValid())
	{
		LOG(ERROR) << "Failed to get Android activity while initializing notifications";
		return;
	}

	QJniObject::callStaticMethod<void>(
		NOTIFICATION_HELPER,
		"initialize",
		"(Landroid/app/Activity;)V",
		activity.object());

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
		LOG(ERROR) << "Failed to initialize Android notifications";
}

void SetBackgroundLocationTrackingEnabled(bool enabled)
{
	const auto activity = Activity();
	if (!activity.isValid())
	{
		LOG(ERROR) << "Failed to get Android activity while updating background location tracking";
		return;
	}

	QJniObject::callStaticMethod<void>(
		LOCATION_FOREGROUND_SERVICE,
		"setEnabled",
		"(Landroid/content/Context;Z)V",
		activity.object(),
		static_cast<jboolean>(enabled));

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
		LOG(ERROR) << "Failed to update Android background location tracking";
}

void ShowPhotoProximityNotification(const QString title, const QString body, int photoId)
{
	const auto activity = Activity();
	if (!activity.isValid())
	{
		LOG(ERROR) << "Failed to get Android activity while showing notification";
		return;
	}

	QJniObject::callStaticMethod<void>(
		NOTIFICATION_HELPER,
		"showPhotoProximityNotification",
		"(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;I)V",
		activity.object(),
		QJniObject::fromString(title).object(),
		QJniObject::fromString(body).object(),
		static_cast<jint>(photoId));

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
		LOG(ERROR) << "Failed to show Android proximity notification";
}

bool SaveScreenshotToGallery(const QString filePath)
{
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists())
	{
		LOG(ERROR) << "File does not exist: " << filePath.toStdString();
		return false;
	}

	try
	{
		// Use MediaStore API to insert image into gallery (Android 10+)
		const auto activity = Activity();

		if (!activity.isValid())
		{
			LOG(ERROR) << "Failed to get Android activity";
			return false;
		}

		const auto contentResolver = activity.callObjectMethod(
			"getContentResolver",
			"()Landroid/content/ContentResolver;");

		if (!contentResolver.isValid())
		{
			LOG(ERROR) << "Failed to get ContentResolver";
			return false;
		}

		// Create ContentValues
		const QJniObject contentValues("android/content/ContentValues");
		if (!contentValues.isValid())
		{
			LOG(ERROR) << "Failed to create ContentValues";
			return false;
		}

		// Set MediaStore values
		const auto displayName = fileInfo.fileName();
		const auto mimeType = "image/png";
		const auto relativePath = "Pictures/PastViewer";

		contentValues.callMethod<void>("put", "(Ljava/lang/String;Ljava/lang/String;)V", QJniObject::fromString("_display_name").object(), QJniObject::fromString(displayName).object());
		contentValues.callMethod<void>("put", "(Ljava/lang/String;Ljava/lang/String;)V", QJniObject::fromString("mime_type").object(), QJniObject::fromString(mimeType).object());
		contentValues.callMethod<void>("put", "(Ljava/lang/String;Ljava/lang/String;)V", QJniObject::fromString("relative_path").object(), QJniObject::fromString(relativePath).object());

		// Get MediaStore.EXTERNAL_CONTENT_URI for images
		const auto mediaStoreUri = QJniObject::callStaticObjectMethod(
			"android/provider/MediaStore$Images$Media",
			"getContentUri",
			"(Ljava/lang/String;)Landroid/net/Uri;",
			QJniObject::fromString("external").object());

		if (!mediaStoreUri.isValid())
		{
			LOG(ERROR) << "Failed to get MediaStore URI";
			return false;
		}

		// Insert into MediaStore (this creates the entry)
		const auto uri = contentResolver.callObjectMethod(
			"insert",
			"(Landroid/net/Uri;Landroid/content/ContentValues;)Landroid/net/Uri;",
			mediaStoreUri.object(),
			contentValues.object());

		if (!uri.isValid())
		{
			LOG(ERROR) << "Failed to insert into MediaStore";
			return false;
		}

		// Open output stream from the MediaStore URI
		const auto outputStream = contentResolver.callObjectMethod(
			"openOutputStream",
			"(Landroid/net/Uri;)Ljava/io/OutputStream;",
			uri.object());

		if (!outputStream.isValid())
		{
			LOG(ERROR) << "Failed to open output stream from MediaStore URI";
			return false;
		}

		// Read source file
		QFile sourceFile(filePath);
		if (!sourceFile.open(QIODevice::ReadOnly))
		{
			LOG(ERROR) << "Failed to open source file: " << filePath.toStdString();
			return false;
		}

		auto data = sourceFile.readAll();
		sourceFile.close();

		// Write to MediaStore output stream using JNI
		QJniEnvironment env;
		const auto byteArray = env->NewByteArray(data.size());
		env->SetByteArrayRegion(byteArray, 0, data.size(), reinterpret_cast<const jbyte *>(data.constData()));

		outputStream.callMethod<void>("write", "([B)V", byteArray);
		outputStream.callMethod<void>("flush", "()V");
		outputStream.callMethod<void>("close", "()V");

		env->DeleteLocalRef(byteArray);

		LOG(INFO) << "Successfully saved screenshot to gallery via MediaStore: " << uri.toString().toStdString();
		return true;
	}
	catch (...)
	{
		LOG(ERROR) << "Exception while saving to MediaStore";
		return false;
	}
}

bool ShareImage(const QString filePath)
{
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists())
	{
		LOG(ERROR) << "File does not exist: " << filePath.toStdString();
		return false;
	}

	const auto activity = Activity();
	if (!activity.isValid())
	{
		LOG(ERROR) << "Failed to get Android activity";
		return false;
	}

	const auto ok = QJniObject::callStaticMethod<jboolean>(
		SHARE_HELPER,
		"shareImage",
		"(Landroid/app/Activity;Ljava/lang/String;)Z",
		activity.object(),
		QJniObject::fromString(filePath).object());
	if (!ok)
		LOG(ERROR) << "Failed to share image: " << filePath.toStdString();

	return ok;
}

bool SupportsNativeTipPurchases()
{
	return true;
}

void InitializeTipPurchases(QStringList productIds)
{
	if (productIds.isEmpty())
		return;

	const auto activity = Activity();
	if (!activity.isValid())
	{
		LOG(ERROR) << "Failed to get Android activity while initializing Google Play Billing";
		return;
	}

	QJniObject::callStaticMethod<void>(
		GOOGLE_PLAY_BILLING_HELPER,
		"initialize",
		"(Landroid/app/Activity;Ljava/lang/String;)V",
		activity.object(),
		QJniObject::fromString(productIds.join(QLatin1Char(','))).object());

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
		LOG(ERROR) << "Failed to initialize Google Play Billing";
}

void LoadTipProducts(QStringList productIds, TipProductsHandler handler)
{
	bool requestInProgress = false;
	{
		const std::scoped_lock lock(tipCallbackMutex);
		requestInProgress = static_cast<bool>(tipProductsHandler);
		// Replace any in-flight waiter so the outstanding Play query delivers to the latest caller.
		tipProductsHandler = std::move(handler);
	}
	if (requestInProgress)
		return;

	const auto activity = Activity();
	if (!activity.isValid())
	{
		TipProductsHandler failedHandler;
		{
			const std::scoped_lock lock(tipCallbackMutex);
			failedHandler = std::move(tipProductsHandler);
		}
		DispatchToQt(std::move(failedHandler), QVector<TipProduct> {}, QStringLiteral("Android activity is unavailable."));
		return;
	}

	QJniObject::callStaticMethod<void>(
		GOOGLE_PLAY_BILLING_HELPER,
		"loadProducts",
		"(Landroid/app/Activity;Ljava/lang/String;)V",
		activity.object(),
		QJniObject::fromString(productIds.join(QLatin1Char(','))).object());

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
	{
		TipProductsHandler failedHandler;
		{
			const std::scoped_lock lock(tipCallbackMutex);
			failedHandler = std::move(tipProductsHandler);
		}
		DispatchToQt(std::move(failedHandler), QVector<TipProduct> {}, QStringLiteral("Could not call Google Play Billing."));
	}
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
		handler(TipPurchaseResult::Failed, QStringLiteral("A Google Play purchase is already in progress."));
		return;
	}

	const auto activity = Activity();
	if (!activity.isValid())
	{
		TipPurchaseHandler failedHandler;
		{
			const std::scoped_lock lock(tipCallbackMutex);
			failedHandler = std::move(tipPurchaseHandler);
		}
		DispatchToQt(std::move(failedHandler), TipPurchaseResult::Failed, QStringLiteral("Android activity is unavailable."));
		return;
	}

	QJniObject::callStaticMethod<void>(
		GOOGLE_PLAY_BILLING_HELPER,
		"purchaseProduct",
		"(Landroid/app/Activity;Ljava/lang/String;)V",
		activity.object(),
		QJniObject::fromString(productId).object());

	QJniEnvironment env;
	if (env.checkAndClearExceptions())
	{
		TipPurchaseHandler failedHandler;
		{
			const std::scoped_lock lock(tipCallbackMutex);
			failedHandler = std::move(tipPurchaseHandler);
		}
		DispatchToQt(std::move(failedHandler), TipPurchaseResult::Failed, QStringLiteral("Could not call Google Play Billing."));
	}
}

} // namespace PlatformDependentLogic
