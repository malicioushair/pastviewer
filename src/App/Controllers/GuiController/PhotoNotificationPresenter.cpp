#include "PhotoNotificationPresenter.h"

#include <utility>

namespace PhotoNotificationPresenter {

void PresentPhotoApproach(
	const Qt::ApplicationState applicationState,
	QString title,
	QString body,
	const int photoId,
	const ShowNotificationHandler & showNotification)
{
	if (applicationState == Qt::ApplicationActive || !showNotification)
		return;

	showNotification(std::move(title), std::move(body), photoId);
}

}
