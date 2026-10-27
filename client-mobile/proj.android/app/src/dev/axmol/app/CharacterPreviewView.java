/****************************************************************************
 * Menu character preview (port of client/src/ui/opponentDisplay.ts).
 *
 * The web client renders a live player in the menu (idle pose + melee idles).
 * The native menu is a Java overlay, so this view draws three pre-composed
 * layers produced by tools/build-character-preview.mjs:
 *
 *   img/preview/outfit_<outfit>.png  body + left hand + backpack (fists pose)
 *   img/preview/hand_<outfit>.png    right hand, separate so it can swing
 *   img/preview/melee_<melee>.png    selected melee, pivoted on the right hand
 *
 * The layers share a 512x512 canvas whose character centre is (256,256) and
 * whose right-hand bone is at (312,207). A slow vertical bob plus an occasional
 * fists/melee attack (HandR -> animData.ts attack keyframe) keeps the avatar
 * from looking frozen.
 ****************************************************************************/
package dev.axmol.app;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.Log;
import android.view.View;

import java.io.InputStream;

public class CharacterPreviewView extends View {
    private static final String TAG = "CharacterPreview";

    private static final int SOURCE = 512;
    private static final float CENTER = 256.0f;
    // IdlePoses.fists HandR (14,12.25) and the fists attack keyframe
    // (29.75,1.75) from client/src/animData.ts, in 4x canvas pixels.
    private static final float HAND_X = 312.0f;
    private static final float HAND_Y = 207.0f;
    private static final float ATTACK_X = 375.0f;
    private static final float ATTACK_Y = 249.0f;
    // The character only occupies the middle of the 512 canvas; zoom in so it
    // reads at menu size without regenerating the layers.
    private static final float ZOOM = 1.9f;

    private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final java.util.Random mRandom = new java.util.Random();
    private Bitmap mOutfit;
    private Bitmap mHand;
    private Bitmap mMelee;
    private String mOutfitType = "";
    private String mMeleeType = "";
    private String mLastLoadoutJson = "";
    private long mStartMs = System.currentTimeMillis();
    private long mLastPollMs = 0;
    private long mNextSwingMs = System.currentTimeMillis() + 1800;
    private long mSwingStartMs = -1;

    public CharacterPreviewView(Context context) {
        super(context);
    }

    public void setLoadout(String outfit, String melee) {
        String nextOutfit = outfit == null ? "" : outfit;
        String nextMelee = melee == null ? "" : melee;
        if (nextOutfit.equals(mOutfitType) && nextMelee.equals(mMeleeType)) {
            return;
        }
        mOutfitType = nextOutfit;
        mMeleeType = nextMelee;
        mOutfit = loadLayer("outfit", mOutfitType);
        mHand = loadLayer("hand", mOutfitType);
        mMelee = loadLayer("melee", mMeleeType);
        invalidate();
    }

    private Bitmap loadLayer(String kind, String type) {
        if (type == null || type.isEmpty()) return null;
        // Fists has no world art in the web preview either.
        if ("melee".equals(kind) && "fists".equals(type)) return null;
        String asset = "img/preview/" + kind + "_" + type + ".png";
        try (InputStream in = getContext().getAssets().open(asset)) {
            return BitmapFactory.decodeStream(in);
        } catch (Exception e) {
            // Not every melee has world art (fists) and an unknown/custom item
            // can be missing from the generated table; draw the other layer.
            Log.w(TAG, "asset " + asset + ": " + e.getMessage());
            return null;
        }
    }

    // The loadout screen writes config on every tap; poll it while the menu
    // (and the panel in front) is up so the avatar follows live like the web.
    private void pollLoadout() {
        long now = System.currentTimeMillis();
        if (now - mLastPollMs < 350) return;
        mLastPollMs = now;
        String json = SurvBridge.getLoadoutJson();
        if (json == null || json.equals(mLastLoadoutJson)) return;
        mLastLoadoutJson = json;
        setLoadout(Json.peekString(json, "outfit"), Json.peekString(json, "melee"));
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        pollLoadout();
        if (mOutfit == null) return;

        long now = System.currentTimeMillis();
        float handX = HAND_X;
        float handY = HAND_Y;
        if (mSwingStartMs < 0 && now >= mNextSwingMs) {
            mSwingStartMs = now;
        }
        if (mSwingStartMs >= 0) {
            float t = (now - mSwingStartMs) / 1000.0f;
            float p;
            if (t < 0.10f) {
                // Wind-up: ease towards the damage-time keyframe.
                float u = t / 0.10f;
                p = 1.0f - (1.0f - u) * (1.0f - u);
            } else if (t < 0.38f) {
                // Follow-through back to the idle pose.
                p = 1.0f - (t - 0.10f) / 0.28f;
            } else {
                mSwingStartMs = -1;
                mNextSwingMs = now + 1800 + mRandom.nextInt(1800);
                p = 0.0f;
            }
            handX = HAND_X + (ATTACK_X - HAND_X) * p;
            handY = HAND_Y + (ATTACK_Y - HAND_Y) * p;
        }

        float scale = (getWidth() / (float) SOURCE) * ZOOM;
        float bob = (float) Math.sin((now - mStartMs) / 1400.0) * 2.0f;
        float dx = getWidth() * 0.5f - CENTER * scale;
        float dy = getHeight() * 0.5f - CENTER * scale + bob;

        canvas.save();
        canvas.clipRect(0, 0, getWidth(), getHeight());
        RectF dst = new RectF(dx, dy, dx + SOURCE * scale, dy + SOURCE * scale);
        canvas.drawBitmap(mOutfit, null, dst, mPaint);

        // The melee layer is centred on the hand; the hand layer keeps its idle
        // position in the canvas, so each shifts by its own source origin.
        if (mMelee != null) {
            float mx = dx + (handX - CENTER) * scale;
            float my = dy + (handY - CENTER) * scale;
            RectF meleeDst = new RectF(mx, my, mx + SOURCE * scale, my + SOURCE * scale);
            canvas.drawBitmap(mMelee, null, meleeDst, mPaint);
        }
        if (mHand != null) {
            float hx = dx + (handX - HAND_X) * scale;
            float hy = dy + (handY - HAND_Y) * scale;
            RectF handDst = new RectF(hx, hy, hx + SOURCE * scale, hy + SOURCE * scale);
            canvas.drawBitmap(mHand, null, handDst, mPaint);
        }
        canvas.restore();

        // Keep the idle bob alive while the menu is visible.
        if (isShown()) {
            postInvalidateOnAnimation();
        }
    }
}
