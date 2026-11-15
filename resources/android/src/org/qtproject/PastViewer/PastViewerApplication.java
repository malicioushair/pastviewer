package org.qtproject.PastViewer;

import com.posthog.PostHog;
import com.posthog.android.PostHogAndroid;
import com.posthog.android.PostHogAndroidConfig;

import org.qtproject.qt.android.bindings.QtApplication;

import java.util.Map;

public class PastViewerApplication extends QtApplication {
    private static final String LOG_SERVICE_NAME = "pastviewer-android";
    private static boolean postHogInitialized;

    @Override
    public void onCreate() {
        super.onCreate();

        if (BuildConfig.POSTHOG_PROJECT_TOKEN == null) {
            reportMissingConfiguration("POSTHOG_PROJECT_TOKEN");
            return;
        }
        if (BuildConfig.POSTHOG_HOST == null) {
            reportMissingConfiguration("POSTHOG_HOST");
            return;
        }

        PostHogAndroidConfig config = new PostHogAndroidConfig(
            BuildConfig.POSTHOG_PROJECT_TOKEN,
            BuildConfig.POSTHOG_HOST
        );
        config.getErrorTrackingConfig().setAutoCapture(true);
        config.getLogs().setServiceName(LOG_SERVICE_NAME);
        PostHogAndroid.setup(this, config);
        postHogInitialized = true;
        logInfo("PostHog log capture initialized", null);
    }

    public static void capture(String event, Map<String, Object> properties) {
        if (!postHogInitialized) {
            return;
        }
        try {
            PostHogAndroid.getInstance().capture(event, properties);
        } catch (RuntimeException ignored) {
        }
    }

    public static void capture(String event) {
        if (!postHogInitialized) {
            return;
        }
        try {
            PostHogAndroid.getInstance().capture(event);
        } catch (RuntimeException ignored) {
        }
    }

    public static void logInfo(String message, Map<String, Object> attributes) {
        if (!postHogInitialized) {
            return;
        }
        try {
            PostHog.Companion.getLogger().info(message, attributes);
        } catch (RuntimeException ignored) {
        }
    }

    private void reportMissingConfiguration(String variableName) {
        if (BuildConfig.DEBUG) {
            throw new IllegalStateException(
                variableName + " variable required by PostHog is missing or un-configured, this causes events to be silently missed. This error stops appearing once " + variableName + " is configured"
            );
        }
    }
}
