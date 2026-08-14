package org.qtproject.PastViewer;

import android.content.Intent;

import org.qtproject.qt.android.bindings.QtActivity;

public class PastViewerActivity extends QtActivity {
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        NotificationHelper.handleNotificationIntent(intent);
    }
}
