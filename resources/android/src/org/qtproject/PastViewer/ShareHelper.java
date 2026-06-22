package org.qtproject.PastViewer;

import android.app.Activity;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;

import androidx.core.content.FileProvider;

import java.io.File;

public final class ShareHelper {
    private ShareHelper() {}

    public static boolean shareImage(Activity activity, String filePath) {
        if (activity == null || filePath == null || filePath.isEmpty()) {
            return false;
        }

        File file = new File(filePath);
        if (!file.exists()) {
            return false;
        }

        activity.runOnUiThread(() -> {
            Uri uri = FileProvider.getUriForFile(
                activity,
                activity.getPackageName() + ".qtprovider",
                file);
            Intent intent = new Intent(Intent.ACTION_SEND);
            intent.setType("image/png");
            intent.putExtra(Intent.EXTRA_STREAM, uri);
            intent.setClipData(ClipData.newUri(activity.getContentResolver(), "PastViewer image", uri));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            activity.startActivity(Intent.createChooser(intent, "Share image"));
        });
        return true;
    }
}
