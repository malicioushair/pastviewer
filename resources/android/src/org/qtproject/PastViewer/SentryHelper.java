package org.qtproject.PastViewer;

import android.content.Context;
import android.util.Log;
import io.sentry.Sentry;
import io.sentry.android.core.SentryAndroid;

public class SentryHelper {
    private static boolean initialized = false;

    public static void init(Context context, String dsn, String release) {
        if (initialized) {
            return;
        }

        SentryAndroid.init(context, options -> {
            options.setDsn(dsn);
            options.setRelease(release);
            options.setDebug(true); // Enable debug mode for testing
            options.setSampleRate(1.0); // 100% sample rate for testing

            options.setEnableAutoSessionTracking(true);
            options.setAttachStacktrace(true);

            // Note: Native crash handling is enabled by default in SentryAndroid.init()
            // No explicit method call needed for SDK 7.15.0
        });

        initialized = true;
    }

    public static void captureMessage(String message) {
        if (initialized) {
            Sentry.captureMessage(message);
        }
    }

    public static void captureException(Throwable throwable) {
        if (initialized) {
            Sentry.captureException(throwable);
        }
    }

    public static void captureException(String message, String type) {
        captureException(message, type, null);
    }

    public static void captureException(String message, String type, String cppStackTrace) {
        if (initialized) {
            // Create exception with proper stack trace
            RuntimeException exception = new RuntimeException(message + " (C++ exception type: " + type + ")");

            // If we have a C++ stack trace, add it as extra data
            if (cppStackTrace != null && !cppStackTrace.isEmpty()) {
                // Use Sentry.configureScope to add extra context for this event
                Sentry.configureScope(scope -> {
                    scope.setExtra("cpp_stacktrace", cppStackTrace);
                    scope.setTag("exception_source", "cpp");
                });

                // Capture the exception - it will include the extra data
                Sentry.captureException(exception);

                // Clear the extra data for next event
                Sentry.configureScope(scope -> {
                    scope.removeExtra("cpp_stacktrace");
                    scope.removeTag("exception_source");
                });
            } else {
                // No C++ stack trace, just capture the Java exception
                Sentry.captureException(exception);
            }
        } else {
            Log.w("SentryHelper", "Sentry not initialized, cannot capture exception");
        }
    }

    public static void addBreadcrumb(String message, String level) {
        if (initialized) {
            io.sentry.Breadcrumb breadcrumb = new io.sentry.Breadcrumb();
            breadcrumb.setMessage(message);
            breadcrumb.setCategory("glog");

            switch (level.toLowerCase()) {
                case "debug":
                    breadcrumb.setLevel(io.sentry.SentryLevel.DEBUG);
                    break;
                case "info":
                    breadcrumb.setLevel(io.sentry.SentryLevel.INFO);
                    break;
                case "warning":
                    breadcrumb.setLevel(io.sentry.SentryLevel.WARNING);
                    break;
                case "error":
                    breadcrumb.setLevel(io.sentry.SentryLevel.ERROR);
                    break;
                default:
                    breadcrumb.setLevel(io.sentry.SentryLevel.INFO);
                    break;
            }

            Sentry.addBreadcrumb(breadcrumb);
        }
    }

    public static void flush() {
        if (initialized) {
            Sentry.flush(2000);
        }
    }
}

