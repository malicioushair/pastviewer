package org.qtproject.PastViewer;

import android.Manifest;
import android.app.Activity;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;

import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;

public final class NotificationHelper {
    private static final String CHANNEL_ID = "photo_proximity";
    private static final int PERMISSION_REQUEST_CODE = 2026;
    private static final int NOTIFICATION_ID_PREFIX = 0x50000000;

    private NotificationHelper() {
    }

    public static void initialize(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "Nearby historical photos",
                    NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription("Alerts when you are within 10 m of a historical photo");

            NotificationManager manager = activity.getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && ActivityCompat.checkSelfPermission(activity, Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    activity,
                    new String[]{Manifest.permission.POST_NOTIFICATIONS},
                    PERMISSION_REQUEST_CODE);
        }
    }

    public static void showPhotoProximityNotification(
            Activity activity,
            String title,
            String body,
            int photoId) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && ActivityCompat.checkSelfPermission(activity, Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            return;
        }

        NotificationManagerCompat manager = NotificationManagerCompat.from(activity);
        if (!manager.areNotificationsEnabled()) {
            return;
        }

        Intent launchIntent = activity.getPackageManager().getLaunchIntentForPackage(activity.getPackageName());
        PendingIntent contentIntent = null;
        if (launchIntent != null) {
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            int flags = PendingIntent.FLAG_UPDATE_CURRENT;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                flags |= PendingIntent.FLAG_IMMUTABLE;
            }
            contentIntent = PendingIntent.getActivity(activity, photoId, launchIntent, flags);
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(activity, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_notification)
                .setContentTitle(title)
                .setContentText(body)
                .setStyle(new NotificationCompat.BigTextStyle().bigText(body))
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setAutoCancel(true);
        if (contentIntent != null) {
            builder.setContentIntent(contentIntent);
        }

        manager.notify(NOTIFICATION_ID_PREFIX ^ photoId, builder.build());
    }
}
