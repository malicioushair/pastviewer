#pragma once

#include <functional>

#include <QString>
#include <QtCore/qnamespace.h>

namespace PhotoNotificationPresenter {

using ShowNotificationHandler = std::function<void(QString title, QString body, int photoId)>;

void PresentPhotoApproach(
	Qt::ApplicationState applicationState,
	QString title,
	QString body,
	int photoId,
	const ShowNotificationHandler & showNotification);

}
