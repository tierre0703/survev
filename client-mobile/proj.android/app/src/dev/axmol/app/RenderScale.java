/****************************************************************************
 * RenderScale.java - the "Low GFX" render-resolution tier (Perf/polish).
 *
 * The web client can drop texture resolution (`highResTex`); on native the
 * equivalent fill-rate saving is a smaller EGL surface: `SurfaceHolder
 * .setFixedSize()` makes the GLSurfaceView render into a smaller buffer that the
 * compositor upscales to the view, so the world/HUD layout is unchanged while
 * the GPU shades fewer pixels (and the screenshot looks slightly softer).
 *
 * The persisted value lives in the C++ config blob (`renderScale`, applied by
 * `AndroidUiBridge::setScenes` at startup); the pause dialog calls apply() and
 * `SurvBridge.setConfigValue()` when the user toggles it.
 ****************************************************************************/
package dev.axmol.app;

import android.util.Log;

import dev.axmol.lib.AxmolGLSurfaceView;

public final class RenderScale {
    private static final String TAG = "RenderScale";

    /** Native resolution (no fixed surface size). */
    public static final float FULL = 1.0f;
    /** "Low GFX": 75% of the view resolution (~44% of the pixels). */
    public static final float LOW = 0.75f;

    private RenderScale() {}

    /** `renderScale` from the config blob (FULL when missing/invalid). */
    public static float persisted() {
        String config = SurvBridge.getConfigJson();
        return normalize(Json.peekFloat(config, "renderScale", FULL));
    }

    /** Clamp to a sane range and snap the full-resolution case. */
    public static float normalize(float value) {
        if (!(value > 0.0f)) return FULL; // also catches NaN
        if (value >= 0.999f) return FULL;
        return Math.max(0.25f, value);
    }

    /**
     * Applies `scale` to the engine's GL surface. Safe to call from the UI
     * thread at any time; the actual resize is posted until the view is
     * measured (layout can lag the first frame by a few milliseconds).
     */
    public static void apply(final float requested) {
        final float scale = normalize(requested);
        final AxmolGLSurfaceView view = AxmolGLSurfaceView.getInstance();
        if (view == null) {
            Log.w(TAG, "no GL surface yet; render scale " + scale + " not applied");
            return;
        }
        view.post(new Runnable() {
            private int attempts = 0;

            @Override
            public void run() {
                final int width = view.getWidth();
                final int height = view.getHeight();
                if ((width <= 0 || height <= 0) && attempts++ < 10) {
                    view.postDelayed(this, 50);
                    return;
                }
                if (width <= 0 || height <= 0) {
                    Log.w(TAG, "view never got measured; keeping render scale " + scale);
                    return;
                }
                if (scale >= 0.999f) {
                    // (0, 0) clears the fixed size -> the surface follows the view.
                    view.getHolder().setFixedSize(0, 0);
                    Log.i(TAG, "render scale FULL (" + width + "x" + height + ")");
                } else {
                    final int surfaceWidth = Math.max(1, Math.round(width * scale));
                    final int surfaceHeight = Math.max(1, Math.round(height * scale));
                    view.getHolder().setFixedSize(surfaceWidth, surfaceHeight);
                    Log.i(TAG, "render scale " + scale + " (" + surfaceWidth + "x"
                            + surfaceHeight + " from " + width + "x" + height + ")");
                }
            }
        });
    }
}
