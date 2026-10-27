/****************************************************************************
 * ItemArt.java - loadout item art shared by the Java screens.
 *
 * `Content/img/loadout/<type>.png` is generated from the web SVGs by
 * `tools/build-loadout-icons.mjs` (melee rotation/mirror and the outfit tint
 * source baked/recorded there), and `Content/gui/loadout-*.png` are the 4x
 * category icons. Both the customize grid and the pass reward art decode the
 * same files, so the bitmap cache and the outfit tint matrix live here.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.util.LruCache;
import android.widget.ImageView;

import java.io.InputStream;

final class ItemArt {
    private ItemArt() {}

    // Decoded item art is shared across refreshes; keep roughly 4 MB of bitmaps.
    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(4 * 1024 * 1024) {
        @Override
        protected int sizeOf(String key, Bitmap value) {
            return value.getByteCount();
        }
    };

    /** Decodes an asset (e.g. "img/loadout/outfitParma.png"); null when missing. */
    static Bitmap load(Activity activity, String path) {
        return load(activity, path, 1);
    }

    static Bitmap load(Activity activity, String path, int sampleSize) {
        final int sample = Math.max(1, sampleSize);
        final String key = path + "@" + sample;
        Bitmap cached = CACHE.get(key);
        if (cached != null) return cached;
        try (InputStream in = activity.getAssets().open(path)) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inSampleSize = sample;
            Bitmap bitmap = BitmapFactory.decodeStream(in, null, options);
            if (bitmap != null) CACHE.put(key, bitmap);
            return bitmap;
        } catch (Exception e) {
            return null;
        }
    }

    /** The item's generated art tile (`img/loadout/<type>.png`). */
    static Bitmap loadItem(Activity activity, String type) {
        if (type == null || type.isEmpty()) return null;
        return load(activity, "img/loadout/" + type + ".png");
    }

    /**
     * Web outfit art is a white silhouette with an SVG multiply filter
     * (`helpers.getSvgFilterForTint`); the same matrix works on the PNG.
     */
    static void tint(ImageView view, int tint) {
        if (tint == 0xFFFFFF) {
            view.clearColorFilter();
            return;
        }
        ColorMatrix matrix = new ColorMatrix(new float[] {
                Color.red(tint) / 255f, 0, 0, 0, 0,
                0, Color.green(tint) / 255f, 0, 0, 0,
                0, 0, Color.blue(tint) / 255f, 0, 0,
                0, 0, 0, 1, 0,
        });
        view.setColorFilter(new ColorMatrixColorFilter(matrix));
    }
}
