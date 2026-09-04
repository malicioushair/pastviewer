#include <array>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "App/Controllers/GuiController/PhotoNotificationPresenter.h"

namespace {

struct PresentedNotification
{
	QString title;
	QString body;
	int photoId;
};

}

TEST(PhotoNotificationPresenterTest, SuppressesNotificationWhileApplicationIsActive)
{
	int notificationCount = 0;
	PhotoNotificationPresenter::PresentPhotoApproach(
		Qt::ApplicationActive,
		QStringLiteral("Historical photo nearby"),
		QStringLiteral("You are near \"Test photo\"."),
		42,
		[&](QString, QString, int) {
			++notificationCount;
		});

	EXPECT_EQ(notificationCount, 0);
}

TEST(PhotoNotificationPresenterTest, PresentsNotificationForEveryNonActiveState)
{
	const std::array backgroundStates {
		Qt::ApplicationSuspended,
		Qt::ApplicationHidden,
		Qt::ApplicationInactive,
	};
	std::vector<PresentedNotification> notifications;

	for (const auto state : backgroundStates)
		PhotoNotificationPresenter::PresentPhotoApproach(
			state,
			QStringLiteral("Historical photo nearby"),
			QStringLiteral("You are near \"Terazije in 1930\"."),
			42,
			[&](QString title, QString body, int photoId) {
				notifications.push_back({ std::move(title), std::move(body), photoId });
			});

	ASSERT_EQ(notifications.size(), backgroundStates.size());
	for (const auto & notification : notifications)
	{
		EXPECT_EQ(notification.title, QStringLiteral("Historical photo nearby"));
		EXPECT_EQ(notification.body, QStringLiteral("You are near \"Terazije in 1930\"."));
		EXPECT_EQ(notification.photoId, 42);
	}
}

TEST(PhotoNotificationPresenterTest, IgnoresMissingNotificationHandler)
{
	EXPECT_NO_THROW(PhotoNotificationPresenter::PresentPhotoApproach(
		Qt::ApplicationHidden,
		QStringLiteral("Historical photo nearby"),
		QStringLiteral("You are near \"Test photo\"."),
		42,
		{}));
}
