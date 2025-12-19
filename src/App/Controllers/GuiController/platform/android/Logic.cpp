#include "App/Controllers/GuiController/platform/Logic.h"

#include <QDir>
#include <QFileInfo>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStandardPaths>

#include "glog/logging.h"

namespace PlatformDependentLogic {

bool SaveScreenshotToGallery(const QString & filePath)
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
		const auto activity = QJniObject::callStaticObjectMethod(
			"org/qtproject/qt/android/QtNative",
			"activity",
			"()Landroid/app/Activity;");

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
		if (!QFile::remove(filePath))
			LOG(WARNING) << "Failed to delete original file after saving to gallery: " << filePath.toStdString();
		else
			LOG(INFO) << "Successfully removed the original image file: " << filePath.toStdString();

		return true;
	}
	catch (...)
	{
		LOG(ERROR) << "Exception while saving to MediaStore";
		return false;
	}
}

} // namespace PlatformDependentLogic