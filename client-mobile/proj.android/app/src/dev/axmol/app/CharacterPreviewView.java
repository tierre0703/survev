/****************************************************************************
 * Menu character preview (port of client/src/ui/opponentDisplay.ts).
 *
 * The web client renders a live player in the menu (idle pose + melee idles).
 * The native menu is a Java overlay, so this view composes pre-rasterized
 * layers produced by tools/build-character-preview.mjs:
 *
 *   img/preview/body_<outfit>.png     outfit base + backpack
 *   img/preview/hand_l_<outfit>.png   left hand, centred on its canvas
 *   img/preview/hand_r_<outfit>.png   right hand, centred on its canvas
 *   img/preview/melee_<melee>.png     selected melee, anchor-centred (rot+scale)
 *
 * The layers share a 512x512 canvas whose character centre is (256,256) and
 * whose scale is 4 px per world unit (1 world unit = 4 px). The skeleton comes
 * from GeneratedPreviewAnims (tools/codegen_preview_anims.mjs):
 *
 *   - the selected melee def's `anim.idlePose` (animData.ts IdlePoses) gives the
 *     resting hand bones (so a two-handed axe holds its hands where the web
 *     does, not in the fists pose);
 *   - an occasional melee deploy/idle/attack animation plays exactly like
 *     `LoadoutDisplay.update` (`animIdleTicker`, 50% idle, deploy on a swap),
 *     interpolating the animData.ts keyframes with the same easings.
 *
 * Bone transform (player.ts updateRotation / update()):
 *   container position = pose.pos + rotate(pose.pivot, pose.rot)
 * the melee sprite is a child of the HandR bone: anchor-centred, offset by
 * (MeleeR.pos + worldImg.pos) * worldImg.scale and rotated by
 * worldImg.rot + MeleeR.rot (the layer bakes worldImg.rot + scale).
 * A slow vertical bob keeps the avatar alive between animations.
 ****************************************************************************/
package dev.axmol.app;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.Log;
import android.view.View;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class CharacterPreviewView extends View {
    private static final String TAG = "CharacterPreview";

    private static final int SOURCE = 512;
    private static final float CENTER = 256.0f;
    /** animData.ts / build-character-preview.mjs: one world unit = 4 px. */
    private static final float WORLD_SCALE = 4.0f;
    /** The character only occupies the middle of the 512 canvas. */
    private static final float ZOOM = 1.9f;

    // animData.ts Bones order.
    private static final int BONE_COUNT = 6;
    private static final int BONE_HAND_L = 0;
    private static final int BONE_HAND_R = 1;
    private static final int BONE_MELEE_R = 5;

    /** animData.ts Pose: {pivot, rot, pos}. */
    private static final class Bone {
        float pivotX, pivotY, rot, x, y;
    }

    /** animData.ts keyframe: a bone mask plus the bones the frame names. */
    private static final class Keyframe {
        float time;
        int easing;
        int mask;
        final Bone[] bones = new Bone[BONE_COUNT];
    }

    // Parsed GeneratedPreviewAnims rows, shared by every preview instance.
    private static final Map<String, Bone[]> sPoses = new HashMap<>();
    private static final Map<String, List<Keyframe>> sAnims = new HashMap<>();

    private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Random mRandom = new Random();

    private Bitmap mBody;
    private Bitmap mHandL;
    private Bitmap mHandR;
    private Bitmap mMelee;
    private String mOutfitType = "";
    private String mMeleeType = "";
    private String mLastLoadoutJson = "";
    private boolean mLoadoutReady = false;

    /** Melee def state (GeneratedPreviewAnims row). */
    private String mIdlePose = "fists";
    private String[] mAttackAnims = new String[0];
    private String[] mDeployAnims = new String[0];
    private String[] mIdleAnims = new String[0];
    private boolean mRenderOnHand = false;
    private boolean mMeleeChanged = false;
    // player.ts melee sprite: worldImg rot/pos/scale (the layer bakes rot+scale).
    private float mMeleeRot = 0.0f;
    private float mMeleePosX = 0.0f;
    private float mMeleePosY = 0.0f;
    private float mMeleeScaleX = 1.0f;
    private float mMeleeScaleY = 1.0f;

    // Framing (opponentDisplay.ts getCameraTargetZoom/getCameraLoadoutOffset):
    // fit the visible art (body + melee) instead of a fixed zoom so long melees
    // are never clipped. Boxes are canvas px relative to the layer centre.
    private float[] mBodyBox;
    private float[] mMeleeBox;
    private float mFrameCx = 0.0f;
    private float mFrameCy = 0.0f;
    private float mFrameW = SOURCE;
    private float mFrameH = SOURCE;

    private final Bone[] mIdle = new Bone[BONE_COUNT];
    private final Bone[] mBones = new Bone[BONE_COUNT];

    private String mAnimName = "";
    private List<Keyframe> mAnimFrames;
    private boolean mAnimMirror = false;
    private float mAnimTicker = 0.0f;
    /** opponentDisplay.ts animIdleTicker: seconds until the next idle anim. */
    private float mIdleTicker = 3.0f;

    private final long mStartMs = System.currentTimeMillis();
    private long mLastPollMs = 0;
    private long mLastFrameMs = 0;
    private final float[] mMeleeOffset = new float[2];

    public CharacterPreviewView(Context context) {
        super(context);
        for (int i = 0; i < BONE_COUNT; i++) {
            mIdle[i] = new Bone();
            mBones[i] = new Bone();
        }
        System.arraycopy(pose("fists"), 0, mIdle, 0, BONE_COUNT);
    }

    public void setLoadout(String outfit, String melee) {
        String nextOutfit = outfit == null ? "" : outfit;
        String nextMelee = melee == null || melee.isEmpty() ? "fists" : melee;
        if (nextOutfit.equals(mOutfitType) && nextMelee.equals(mMeleeType)) return;
        boolean changed = !nextMelee.equals(mMeleeType);
        boolean wasReady = mLoadoutReady;
        mOutfitType = nextOutfit;
        mMeleeType = nextMelee;
        mBody = loadLayer("body", mOutfitType);
        mHandL = loadLayer("hand_l", mOutfitType);
        mHandR = loadLayer("hand_r", mOutfitType);
        if (mBody == null) {
            // A missing/custom outfit must not leave the slot empty (the web
            // validates the loadout, the C++ config may be mid-write).
            mBody = loadLayer("body", "outfitBase");
            mHandL = loadLayer("hand_l", "outfitBase");
            mHandR = loadLayer("hand_r", "outfitBase");
        }
        mMelee = loadLayer("melee", mMeleeType);
        mBodyBox = mBody != null ? alphaBounds(mBody) : null;
        mMeleeBox = mMelee != null ? alphaBounds(mMelee) : null;
        applyMeleeDef();
        updateFraming();
        if (wasReady && changed) {
            // opponentDisplay.ts setLoadout: a melee swap restarts the display
            // and forces the deploy animation.
            mAnimName = "";
            mAnimFrames = null;
            mMeleeChanged = true;
            mIdleTicker = 0.0f;
        }
        mLoadoutReady = true;
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
            // can be missing from the generated table; draw the other layers.
            Log.w(TAG, "asset " + asset + ": " + e.getMessage());
            return null;
        }
    }

    private void applyMeleeDef() {
        String[] row = GeneratedPreviewAnims.melee(mMeleeType);
        if (row == null) row = GeneratedPreviewAnims.melee("fists");
        if (row == null) {
            mIdlePose = "fists";
            mAttackAnims = mDeployAnims = mIdleAnims = new String[0];
            mRenderOnHand = false;
        } else {
            mIdlePose = row[1];
            mRenderOnHand = "1".equals(row[2]);
            mMeleeRot = parse(row[3], 0.0f);
            mMeleePosX = parse(row[4], 0.0f);
            mMeleePosY = parse(row[5], 0.0f);
            mMeleeScaleX = parse(row[6], 1.0f);
            mMeleeScaleY = parse(row[7], 1.0f);
            mAttackAnims = splitList(row[8]);
            mDeployAnims = splitList(row[9]);
            mIdleAnims = splitList(row[10]);
        }
        System.arraycopy(pose(mIdlePose), 0, mIdle, 0, BONE_COUNT);
    }

    private static float parse(String value, float fallback) {
        try {
            return Float.parseFloat(value);
        } catch (RuntimeException e) {
            return fallback;
        }
    }

    private static String[] splitList(String value) {
        if (value == null || value.isEmpty()) return new String[0];
        return value.split(",");
    }

    // --- GeneratedPreviewAnims parsing ------------------------------------

    private static Bone[] pose(String name) {
        Bone[] bones = sPoses.get(name);
        if (bones == null) {
            bones = parseBones(GeneratedPreviewAnims.pose(name));
            sPoses.put(name, bones);
        }
        return bones;
    }

    private static Bone[] parseBones(String row) {
        Bone[] bones = new Bone[BONE_COUNT];
        for (int i = 0; i < BONE_COUNT; i++) bones[i] = new Bone();
        if (row == null || row.isEmpty()) return bones;
        String[] parts = row.split("\\|", -1);
        for (int i = 0; i < BONE_COUNT && i < parts.length; i++) {
            String[] v = parts[i].split(",", -1);
            if (v.length < 5) continue;
            try {
                bones[i].pivotX = Float.parseFloat(v[0]);
                bones[i].pivotY = Float.parseFloat(v[1]);
                bones[i].rot = Float.parseFloat(v[2]);
                bones[i].x = Float.parseFloat(v[3]);
                bones[i].y = Float.parseFloat(v[4]);
            } catch (NumberFormatException e) {
                Log.w(TAG, "bad pose bone: " + parts[i]);
            }
        }
        return bones;
    }

    private static List<Keyframe> anim(String name) {
        List<Keyframe> frames = sAnims.get(name);
        if (frames != null) return frames;
        frames = new ArrayList<>();
        String row = GeneratedPreviewAnims.anim(name);
        if (row != null && !row.isEmpty()) {
            for (String part : row.split(";", -1)) {
                String[] v = part.split(",", -1);
                if (v.length < 3 + BONE_COUNT * 5) continue;
                try {
                    Keyframe frame = new Keyframe();
                    frame.time = Float.parseFloat(v[0]);
                    frame.easing = Integer.parseInt(v[1]);
                    frame.mask = Integer.parseInt(v[2]);
                    for (int i = 0; i < BONE_COUNT; i++) {
                        Bone bone = new Bone();
                        int o = 3 + i * 5;
                        bone.pivotX = Float.parseFloat(v[o]);
                        bone.pivotY = Float.parseFloat(v[o + 1]);
                        bone.rot = Float.parseFloat(v[o + 2]);
                        bone.x = Float.parseFloat(v[o + 3]);
                        bone.y = Float.parseFloat(v[o + 4]);
                        frame.bones[i] = bone;
                    }
                    frames.add(frame);
                } catch (NumberFormatException e) {
                    Log.w(TAG, "bad anim frame in " + name);
                }
            }
        }
        sAnims.put(name, frames);
        return frames;
    }

    /** shared/utils/math.ts easings (PoseEasing order in src/render/Defs.h). */
    private static float ease(int kind, float t) {
        switch (kind) {
            case 1:
                return (float) (1.0 - Math.cos(t * Math.PI / 2.0));
            case 2:
                return (float) Math.sin(t * Math.PI / 2.0);
            case 3:
                return (float) ((1.0 - Math.cos(Math.PI * t)) / 2.0);
            case 4:
                return 1.0f - (float) Math.pow(1.0 - t, 4.0);
            case 5:
                return easeOutBounce(t);
            case 6:
                return 1.0f - (1.0f - t) * (1.0f - t);
            default:
                return t;
        }
    }

    private static float easeOutBounce(float t) {
        if (t < 1.0f / 2.75f) return 7.5625f * t * t;
        if (t < 2.0f / 2.75f) {
            float x = t - 1.5f / 2.75f;
            return 7.5625f * x * x + 0.75f;
        }
        if (t < 2.5f / 2.75f) {
            float x = t - 2.25f / 2.75f;
            return 7.5625f * x * x + 0.9375f;
        }
        float x = t - 2.625f / 2.75f;
        return 7.5625f * x * x + 0.984375f;
    }

    // --- animation ---------------------------------------------------------

    /**
     * opponentDisplay.ts: if no animation is playing, count down and then pick
     * a deploy (after a melee swap), idle (50%) or attack animation.
     */
    private void playNextAnim() {
        String[] choices = null;
        if (mMeleeChanged && mDeployAnims.length > 0) {
            choices = mDeployAnims;
        } else if (mRandom.nextFloat() < 0.5f && mIdleAnims.length > 0) {
            choices = mIdleAnims;
        } else if (mAttackAnims.length > 0) {
            choices = mAttackAnims;
        }
        mMeleeChanged = false;
        mIdleTicker = 1.5f + mRandom.nextFloat();
        if (choices == null || choices.length == 0) return;
        String name = choices[mRandom.nextInt(choices.length)];
        List<Keyframe> frames = anim(name);
        if (frames.isEmpty()) return;
        mAnimName = name;
        mAnimFrames = frames;
        mAnimTicker = 0.0f;
        // player.ts selectAnim: only the single-entry "fists" attack mirrors.
        mAnimMirror = "fists".equals(name) && choices.length == 1 && mRandom.nextFloat() < 0.5f;
    }

    /**
     * player.ts updateAnim + the idle-pose blend: keyframes only override the
     * bones they name, everything else rests in the melee's idle pose.
     */
    private void resolveBones(float dt) {
        if (mAnimName.isEmpty()) {
            mIdleTicker -= dt;
            if (mIdleTicker < 0.0f) playNextAnim();
        }
        if (!mAnimName.isEmpty()) {
            mAnimTicker += dt;
            List<Keyframe> frames = mAnimFrames;
            int n = frames.size();
            int aIdx = -1;
            int bIdx = 0;
            while (bIdx < n - 1 && mAnimTicker >= frames.get(bIdx).time) {
                aIdx++;
                bIdx++;
            }
            if (aIdx < 0) aIdx = 0;
            if (bIdx == n - 1 && mAnimTicker >= frames.get(n - 1).time) {
                // web: the final frame ends the anim (game: Anim.None -> idle).
                mAnimName = "";
                mAnimFrames = null;
            } else {
                Keyframe fa = frames.get(aIdx);
                Keyframe fb = frames.get(bIdx);
                float span = fb.time - fa.time;
                float t = span > 0.0f ? Math.min((mAnimTicker - fa.time) / span, 1.0f) : 1.0f;
                // Pose.lerp(weight, idlePose, animPose): before the first
                // keyframe the animation eases in from the idle pose.
                float weight = aIdx == bIdx ? t : 1.0f;
                float e = ease(fb.easing, t);
                for (int i = 0; i < BONE_COUNT; i++) {
                    Bone out = mBones[i];
                    Bone idle = mIdle[i];
                    int src = mAnimMirror ? (i % 2 == 0 ? i + 1 : i - 1) : i;
                    if ((fa.mask & (1 << src)) == 0 || (fb.mask & (1 << src)) == 0) {
                        copyBone(out, idle);
                        continue;
                    }
                    Bone ba = fa.bones[src];
                    Bone bb = fb.bones[src];
                    float pivotX = lerp(ba.pivotX, bb.pivotX, e);
                    float pivotY = lerp(ba.pivotY, bb.pivotY, e);
                    float rot = lerp(ba.rot, bb.rot, e);
                    float x = lerp(ba.x, bb.x, e);
                    float y = lerp(ba.y, bb.y, e);
                    if (mAnimMirror) {
                        pivotY = -pivotY;
                        y = -y;
                        rot = -rot;
                    }
                    out.pivotX = lerp(idle.pivotX, pivotX, weight);
                    out.pivotY = lerp(idle.pivotY, pivotY, weight);
                    out.rot = lerp(idle.rot, rot, weight);
                    out.x = lerp(idle.x, x, weight);
                    out.y = lerp(idle.y, y, weight);
                }
                return;
            }
        }
        for (int i = 0; i < BONE_COUNT; i++) copyBone(mBones[i], mIdle[i]);
    }

    private static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    private static void copyBone(Bone dst, Bone src) {
        dst.pivotX = src.pivotX;
        dst.pivotY = src.pivotY;
        dst.rot = src.rot;
        dst.x = src.x;
        dst.y = src.y;
    }

    // --- framing -----------------------------------------------------------

    /** Bone container offset: pos + rotate(pivot, rot) (player.ts updateRotation). */
    private static float boneOffsetX(Bone bone) {
        return bone.x + (float) (bone.pivotX * Math.cos(bone.rot) - bone.pivotY * Math.sin(bone.rot));
    }

    private static float boneOffsetY(Bone bone) {
        return bone.y + (float) (bone.pivotX * Math.sin(bone.rot) + bone.pivotY * Math.cos(bone.rot));
    }

    /** Alpha bounding box relative to the bitmap centre, or null when empty. */
    private static float[] alphaBounds(Bitmap bmp) {
        int w = bmp.getWidth();
        int h = bmp.getHeight();
        int[] pixels = new int[w * h];
        bmp.getPixels(pixels, 0, w, 0, 0, w, h);
        int minX = w;
        int minY = h;
        int maxX = -1;
        int maxY = -1;
        for (int y = 0; y < h; y++) {
            int row = y * w;
            for (int x = 0; x < w; x++) {
                if ((pixels[row + x] >>> 24) > 8) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
        if (maxX < 0) return null;
        return new float[] {minX - w * 0.5f, minY - h * 0.5f, maxX - w * 0.5f, maxY - h * 0.5f};
    }

    /**
     * opponentDisplay.ts frames the avatar with the loadout camera; the Java
     * view fits the union of the body art and the idle-posed melee art.
     */
    private void updateFraming() {
        float minX = -CENTER;
        float minY = -CENTER;
        float maxX = CENTER;
        float maxY = CENTER;
        if (mBodyBox != null) {
            minX = mBodyBox[0];
            minY = mBodyBox[1];
            maxX = mBodyBox[2];
            maxY = mBodyBox[3];
        }
        if (mMelee != null && mMeleeBox != null) {
            Bone hand = mIdle[BONE_HAND_R];
            meleeOffset(mIdle[BONE_MELEE_R]);
            // The melee layer is a child of the HandR bone container.
            float cx = boneOffsetX(hand) + mMeleeOffset[0];
            float cy = boneOffsetY(hand) + mMeleeOffset[1];
            float canvasX = cx * WORLD_SCALE;
            float canvasY = -cy * WORLD_SCALE;
            minX = Math.min(minX, canvasX + mMeleeBox[0]);
            minY = Math.min(minY, canvasY + mMeleeBox[1]);
            maxX = Math.max(maxX, canvasX + mMeleeBox[2]);
            maxY = Math.max(maxY, canvasY + mMeleeBox[3]);
        }
        mFrameCx = (minX + maxX) * 0.5f;
        mFrameCy = (minY + maxY) * 0.5f;
        mFrameW = Math.max(maxX - minX, 1.0f);
        mFrameH = Math.max(maxY - minY, 1.0f);
    }

    /**
     * The melee sprite is a child of the HandR bone (player.ts updateRotation):
     * anchor-centred, pivot -MeleeR.pivot, rotated by worldImg.rot + MeleeR.rot
     * and offset by (MeleeR.pos + worldImg.pos) * worldImg.scale. The layer
     * bakes worldImg.rot/scale, so only the MeleeR bone is applied here.
     */
    private void meleeOffset(Bone bone) {
        float rot = mMeleeRot + bone.rot;
        double cos = Math.cos(rot);
        double sin = Math.sin(rot);
        float px = (bone.x + mMeleePosX) * mMeleeScaleX;
        float py = (bone.y + mMeleePosY) * mMeleeScaleY;
        mMeleeOffset[0] = (float) (px * cos - py * sin) - bone.pivotX;
        mMeleeOffset[1] = (float) (px * sin + py * cos) - bone.pivotY;
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

    // --- drawing -----------------------------------------------------------

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        pollLoadout();
        if (mBody == null) return;

        long now = System.currentTimeMillis();
        float dt = mLastFrameMs == 0 ? 0.0f : Math.min((now - mLastFrameMs) / 1000.0f, 0.1f);
        mLastFrameMs = now;
        resolveBones(dt);

        float scale = (getWidth() / (float) SOURCE) * ZOOM;
        if (mFrameW > 0.0f && mFrameH > 0.0f) {
            // Never clip a long melee: fit the measured union into the view.
            scale = Math.min(scale, Math.min(getWidth() * 0.94f / mFrameW, getHeight() * 0.94f / mFrameH));
        }
        float bob = (float) Math.sin((now - mStartMs) / 1400.0) * 2.0f;
        float dx = getWidth() * 0.5f - mFrameCx * scale;
        float dy = getHeight() * 0.5f - mFrameCy * scale + bob;

        canvas.save();
        canvas.clipRect(0, 0, getWidth(), getHeight());
        canvas.translate(dx, dy);
        canvas.scale(scale, scale);
        canvas.drawBitmap(mBody, -CENTER, -CENTER, mPaint);
        // player.ts bodyContainer order: left hand, then the right hand (which
        // carries the melee sprite, below or above the hand per renderOnHand).
        drawHand(canvas, mHandL, mBones[BONE_HAND_L], null);
        drawHand(canvas, mHandR, mBones[BONE_HAND_R], mMelee);
        canvas.restore();

        // Keep the idle bob + animation alive while the menu is visible.
        if (isShown()) {
            postInvalidateOnAnimation();
        }
    }

    private void drawHand(Canvas canvas, Bitmap hand, Bone bone, Bitmap melee) {
        if (hand == null && melee == null) return;
        canvas.save();
        // world +y is canvas -y.
        canvas.translate(boneOffsetX(bone) * WORLD_SCALE, -boneOffsetY(bone) * WORLD_SCALE);
        canvas.rotate((float) Math.toDegrees(-bone.rot));
        if (melee != null && !mRenderOnHand) drawMelee(canvas);
        if (hand != null) canvas.drawBitmap(hand, -CENTER, -CENTER, mPaint);
        if (melee != null && mRenderOnHand) drawMelee(canvas);
        canvas.restore();
    }

    private void drawMelee(Canvas canvas) {
        Bone bone = mBones[BONE_MELEE_R];
        meleeOffset(bone);
        canvas.save();
        canvas.translate(mMeleeOffset[0] * WORLD_SCALE, -mMeleeOffset[1] * WORLD_SCALE);
        canvas.rotate((float) Math.toDegrees(-bone.rot));
        canvas.drawBitmap(mMelee, -CENTER, -CENTER, mPaint);
        canvas.restore();
    }
}
