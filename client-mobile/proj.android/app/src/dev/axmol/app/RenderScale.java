/****************************************************************************
 * RenderScale.java - render-resolution (battery) tiers (Perf/polish).
 *
 * The web client can drop texture resolution (`highResTex`); on native the
 * equivalent fill-rate saving is a smaller EGL surface: `SurfaceHolder
 * .setFixedSize()` makes the GLSurfaceView render into a smaller buffer that the
 * compositor upscales to the view, so the world/HUD layout is unchanged while
 * the GPU shades fewer pixels (and the screenshot looks slightly softer).
 *
 * Tiers (cycle in the pause dialog):
 *   Full   1.00  native surface
 *   Medium 0.85  ~72% of the pixels
 *   Low    0.75  ~56% of the pixels ("Low GFX" from the first perf pass)
 *   Potato 0.60  ~36% of the pixels
 *
 * The persisted value lives in the C++ config blob (`renderScale`, applied by
 * `AndroidUiBridge::setScenes` at startup); the pause dialog calls apply() and
 * `SurvBridge.setConfigValue()` when the user cycles the tier.
 ****************************************************************************/
package dev.axmol.app;

import android.util.Log;

import dev.axmol.lib.AxmolGLSurfaceView;

public final class RenderScale {
    private static final String TAG = "RenderScale";

    /** Native resolution (no fixed surface size). */
    public static final float FULL = 1.0f;
    /** Mild battery tier: 85% of the view resolution (~72% of the pixels). */
    public static final float MEDIUM = 0.85f;
    /** "Low GFX": 75% of the view resolution (~56% of the pixels). */
    public static final float LOW = 0.75f;
    /** Aggressive battery tier: 60% of the view resolution (~36% of the pixels). */
    public static final float POTATO = 0.60f;

    /** Cycle order used by the pause dialog. */
    public static final float[] TIERS = {FULL, MEDIUM, LOW, POTATO};
    public static final String[] TIER_NAMES = {"Full", "Medium", "Low", "Potato"};

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

    /** Display name for a tier (custom persisted values keep a percentage). */
    public static String tierName(float value) {
        final float scale = normalize(value);
        for (int i = 0; i < TIERS.length; i++) {
            if (Math.abs(scale - TIERS[i]) < 0.005f) {
                return TIER_NAMES[i];
            }
        }
        return String.format(java.util.Locale.US, "Custom %d%%", Math.round(scale * 100));
    }

    /** Next tier in the cycle (Full -> Medium -> Low -> Potato -> Full). */
    public static float next(float value) {
        final float scale = normalize(value);
        int nearest = 0;
        float best = Float.MAX_VALUE;
        for (int i = 0; i < TIERS.length; i++) {
            final float delta = Math.abs(scale - TIERS[i]);
            if (delta < best) {
                best = delta;
                nearest = i;
            }
        }
        return TIERS[(nearest + 1) % TIERS.length];
    }

    /**
     * Applies `scale` to the engine's GL surface. Safe to call from the UI
     * thread at any time; the actual resize is posted until the view is
     * measured (layout can lag the first frame by a few milliseconds).
     */
    public static void apply(final float requested) {
        final float scale = normalize(requested);
        final String tier = tierName(scale);
        final AxmolGLSurfaceView view = AxmolGLSurfaceView.getInstance();
        if (view == null) {
            Log.w(TAG, "no GL surface yet; render tier " + tier + " (" + scale + ") not applied");
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
                    Log.w(TAG, "view never got measured; keeping render tier " + tier);
                    return;
                }
                if (scale >= 0.999f) {
                    // (0, 0) clears the fixed size -> the surface follows the view.
                    view.getHolder().setFixedSize(0, 0);
                    Log.i(TAG, "render tier " + tier + " (" + width + "x" + height + ")");
                } else {
                    final int surfaceWidth = Math.max(1, Math.round(width * scale));
                    final int surfaceHeight = Math.max(1, Math.round(height * scale));
                    view.getHolder().setFixedSize(surfaceWidth, surfaceHeight);
                    Log.i(TAG, "render tier " + tier + " " + scale + " (" + surfaceWidth + "x"
                            + surfaceHeight + " from " + width + "x" + height + ")");
                }
            }
        });
    }
}
