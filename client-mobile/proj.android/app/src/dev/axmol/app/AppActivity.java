/****************************************************************************
Copyright (c) 2015-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).
 
https://axmol.dev/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.os.Looper;
import android.util.Base64;
import dev.axmol.lib.AxmolActivity;
import dev.axmol.lib.SharedLoader;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;
import android.widget.Button;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.EditText;
import android.widget.ScrollView;
import android.util.Log;
import android.view.Gravity;
import android.util.TypedValue;
import android.graphics.Color;

import java.io.File;
import java.io.FileInputStream;

public class AppActivity extends AxmolActivity {
    private static final String TAG = "AppActivity";
    private static final String NATIVE_CRASH_FILE = "native-crash.txt";

    static {
        // DNT remove, some android simulator require explicit load shared libraries, otherwise will crash
        SharedLoader.load();
    }

    private static AppActivity sCurrent;
    private JavaUiHost mJavaUi;

    /**
     * The live activity, for the bridge's C++ -> Java callbacks (they arrive on
     * the GL thread and must be posted to the UI thread; see SurvBridge.post).
     */
    public static AppActivity current() {
        return sCurrent;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        SurvApi.initialize(this);
        super.setEnableVirtualButton(false);
        installCrashReporter();
        super.onCreate(savedInstanceState);
        // Read the previous run's native report before installing the handler
        // (the diagnostic crash hook faults during install).
        reportPreviousNativeCrash();
        installNativeCrashHandler();
        // Workaround in https://stackoverflow.com/questions/16283079/re-launch-of-activity-on-home-button-but-only-the-first-time/16447508
        if (!isTaskRoot()) {
            // Android launched another instance of the root activity into an existing task
            //  so just quietly finish and go away, dropping the user back into the activity
            //  at the top of the stack (ie: the last state of this task)
            // Don't need to finish it again since it's finished in super.onCreate .
            return;
        }
        // Make sure we're running on Pie or higher to change cutout mode
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            // Enable rendering into the cutout area
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }
        // DO OTHER INITIALIZATION BELOW
        sCurrent = this;
        Log.i(TAG, "AppActivity onCreate, mFrameLayout=" + (mFrameLayout != null ? "OK" : "NULL"));
        installJavaUi();
        handleDeepLink(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleDeepLink(intent);
    }

    /**
     * Incoming VIEW links:
     *   - main.ts `tryJoinGameFromParam`: `?u=<base64 urls>&jt=<token>`
     *   - `survev://team/<code>` and `https://<web>/#<code>` team invites.
     */
    private void handleDeepLink(Intent intent) {
        if (intent == null || intent.getData() == null || mJavaUi == null) {
            return;
        }
        final Uri data = intent.getData();
        final String url = data.toString();
        Log.i(TAG, "deep link " + url);

        String u = data.getQueryParameter("u");
        String jt = data.getQueryParameter("jt");
        if (u != null && jt != null && !jt.isEmpty()) {
            try {
                byte[] decoded;
                try {
                    decoded = Base64.decode(u, Base64.DEFAULT);
                } catch (IllegalArgumentException urlSafe) {
                    decoded = Base64.decode(u, Base64.URL_SAFE | Base64.NO_WRAP);
                }
                String joined = new String(decoded, java.nio.charset.StandardCharsets.UTF_8);
                String[] urls = joined.split(",");
                mJavaUi.openJoinLink(urls, jt);
            } catch (Exception e) {
                Log.w(TAG, "invalid join link: " + e.getMessage());
            }
            return;
        }

        if (url.startsWith("survev://team")) {
            mJavaUi.openTeamInvite(url);
            return;
        }
        if (url.startsWith("https://") || url.startsWith("http://")) {
            // The fragment is the room code in web invite links. Android keeps
            // it in the intent URI, so both this and extractCode() see it.
            String fragment = data.getFragment();
            String code = fragment != null && !fragment.isEmpty()
                    ? fragment
                    : TeamLinks.extractCode(url);
            if (!code.isEmpty()) {
                mJavaUi.openTeamInvite(url);
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (sCurrent == this) {
            sCurrent = null;
        }
        super.onDestroy();
    }

    // ===================== Crash reporting (M8) ============================

    /**
     * Port of `client/src/errorLogs.ts logWindowOnError` for native crashes: a
     * report goes to `/api/report_error` before the default handler tears the
     * process down. A main-thread crash posts synchronously (the game thread
     * will never run the budgeted C++ reporter), a worker crash queues it
     * through the bridge and the app keeps running.
     */
    private void installCrashReporter() {
        final Thread.UncaughtExceptionHandler previous = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable error) {
                try {
                    reportCrash(thread, error);
                } catch (Throwable ignored) {
                    // Never mask the original crash with a reporting failure.
                }
                if (previous != null) {
                    previous.uncaughtException(thread, error);
                }
            }
        });
    }

    private File nativeCrashFile() {
        return new File(getFilesDir(), NATIVE_CRASH_FILE);
    }

    /** NDK faults bypass ART; NativeCrash.cpp writes this file for the next run. */
    private void installNativeCrashHandler() {
        try {
            SurvBridge.installNativeCrashHandler(nativeCrashFile().getAbsolutePath());
        } catch (Throwable t) {
            Log.w(TAG, "native crash handler: " + t);
        }
    }

    private void reportPreviousNativeCrash() {
        final File file = nativeCrashFile();
        if (!file.exists()) return;
        String text = "";
        try (FileInputStream in = new FileInputStream(file)) {
            byte[] buffer = new byte[16384];
            int read = in.read(buffer);
            if (read > 0) {
                text = new String(buffer, 0, read, "UTF-8");
            }
        } catch (Exception e) {
            Log.w(TAG, "read native crash: " + e.getMessage());
        }
        // Delete before reporting so a failed upload never re-sends forever.
        file.delete();
        if (text.isEmpty()) return;
        Log.i(TAG, "native crash report: " + text.length() + " bytes");
        final String data = "{\"message\":\"native crash\",\"stacktrace\":\""
                + SurvApi.escapeJson(text) + "\",\"thread\":\"native\"}";
        new Thread(new Runnable() {
            @Override
            public void run() {
                SurvApi.reportError(SurvApi.apiBaseUrl(), "nativeCrash", data);
            }
        }, "surv-native-crash").start();
    }

    private void reportCrash(Thread thread, Throwable error) {
        final String data = "{\"message\":\"" + SurvApi.escapeJson(String.valueOf(error))
                + "\",\"stacktrace\":\"" + SurvApi.escapeJson(Log.getStackTraceString(error))
                + "\",\"thread\":\"" + SurvApi.escapeJson(thread.getName()) + "\"}";

        boolean mainThread = Looper.myLooper() == Looper.getMainLooper();
        if (!mainThread) {
            // App survives: use the budgeted C++ reporter on the game thread.
            SurvBridge.reportError("windowOnError", data);
            return;
        }
        // Main thread: the process is about to die, so send it here and wait
        // (briefly) for the request to leave.
        final String apiBase = SurvApi.apiBaseUrl();
        Thread sender = new Thread(new Runnable() {
            @Override
            public void run() {
                SurvApi.reportError(apiBase, "windowOnError", data);
            }
        }, "surv-crash-reporter");
        sender.start();
        try {
            sender.join(1500);
        } catch (InterruptedException ignored) {
        }
    }

    @Override
    public boolean onKeyDown(final int pKeyCode, final KeyEvent pKeyEvent) {
        // BACK and the gamepad Start button open the Java pause dialog (or leave
        // the current screen) before the GL surface sees them.
        if (pKeyCode == KeyEvent.KEYCODE_BACK || pKeyCode == KeyEvent.KEYCODE_BUTTON_START) {
            final boolean start = pKeyCode == KeyEvent.KEYCODE_BUTTON_START;
            if (mJavaUi != null && mJavaUi.handleBackPressed()) {
                return true;
            }
            // Gamepad Start has no engine BACK path (axmol only maps the Android
            // BACK key), so open the pause dialog directly while in a match.
            if (start && mJavaUi != null && SurvBridge.isInGame()) {
                mJavaUi.showPause();
                return true;
            }
            // If the Java UI didn't handle it, pass to the GL surface (game control
            // e.g. pause menu).
            return false;
        }
        // P1.6: axmol's GLSurfaceView only forwards DPAD/BACK to the engine, so
        // BT-keyboard/gamepad gameplay keys go through the bridge (GL-thread
        // marshalled).
        if (shouldForwardGameKey(pKeyCode)) {
            SurvBridge.hardwareKey(pKeyCode, true);
            return true;
        }
        return super.onKeyDown(pKeyCode, pKeyEvent);
    }

    @Override
    public boolean onKeyUp(final int pKeyCode, final KeyEvent pKeyEvent) {
        // Start is a one-shot pause toggle: swallow its release.
        if (pKeyCode == KeyEvent.KEYCODE_BUTTON_START) {
            return true;
        }
        if (shouldForwardGameKey(pKeyCode)) {
            SurvBridge.hardwareKey(pKeyCode, false);
            return true;
        }
        return super.onKeyUp(pKeyCode, pKeyEvent);
    }

    /**
     * Android gamepads deliver the sticks/triggers as MotionEvent axes (the
     * buttons arrive as KeyEvents through onKeyDown/onKeyUp). Forward the axes
     * only in a match and never while the pause dialog is up. +Y is up for the
     * native input state, so Android's down-positive axes are negated.
     */
    @Override
    public boolean onGenericMotionEvent(final MotionEvent event) {
        final int source = event.getSource();
        if (SurvBridge.isInGame() && (mJavaUi == null || !mJavaUi.isPauseOpen())
                && ((source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                    || (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
                && event.getAction() == MotionEvent.ACTION_MOVE) {
            final float moveX = axis(event, MotionEvent.AXIS_X);
            final float moveY = -axis(event, MotionEvent.AXIS_Y);
            float aimX = axis(event, MotionEvent.AXIS_Z);
            float aimY = -axis(event, MotionEvent.AXIS_RZ);
            if (Math.abs(aimX) < 0.01f && Math.abs(aimY) < 0.01f) {
                // Some pads expose the right stick as RX/RY instead of Z/RZ.
                aimX = axis(event, MotionEvent.AXIS_RX);
                aimY = -axis(event, MotionEvent.AXIS_RY);
            }
            final float fire = Math.max(axis(event, MotionEvent.AXIS_RTRIGGER),
                    axis(event, MotionEvent.AXIS_GAS));
            SurvBridge.gamepadAxes(moveX, moveY, aimX, aimY, fire);
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    private static float axis(final MotionEvent event, final int axis) {
        final InputDevice.MotionRange range = event.getDevice() != null
                ? event.getDevice().getMotionRange(axis, event.getSource())
                : null;
        float value = event.getAxisValue(axis);
        if (range != null) {
            // Normalise odd-range axes (some pads report triggers as min..max).
            final float min = range.getMin();
            final float max = range.getMax();
            if (max > min) {
                value = (value - min) / (max - min) * 2.0f - 1.0f;
            }
        }
        return value;
    }

    /** In a match, but not while the pause dialog wants DPAD focus navigation. */
    private boolean shouldForwardGameKey(final int keyCode) {
        return SurvBridge.isInGame() && isGameKey(keyCode)
                && !(mJavaUi != null && mJavaUi.isPauseOpen());
    }

    /** Letters, digits, DPAD arrows and gamepad buttons are the default binds. */
    private static boolean isGameKey(final int keyCode) {
        return (keyCode >= KeyEvent.KEYCODE_A && keyCode <= KeyEvent.KEYCODE_Z)
                || (keyCode >= KeyEvent.KEYCODE_0 && keyCode <= KeyEvent.KEYCODE_9)
                // Gamepad buttons (A/B/X/Y, shoulders, triggers, sticks).
                || (keyCode >= KeyEvent.KEYCODE_BUTTON_A && keyCode <= KeyEvent.KEYCODE_BUTTON_MODE)
                || keyCode == KeyEvent.KEYCODE_DPAD_LEFT
                || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT
                || keyCode == KeyEvent.KEYCODE_DPAD_UP
                || keyCode == KeyEvent.KEYCODE_DPAD_DOWN;
    }

    /**
     * Adds the Java UI layer above the axmol GLSurfaceView (ARCHITECTURE.md
     * section 3.2 rule 1). `mFrameLayout` already exists here because
     * `super.onCreate` ran AxmolActivity.init(); the container is added after
     * the GL surface so the Java screens draw on top of the game canvas.
     */
    private void installJavaUi() {
        Log.i(TAG, "installJavaUi called, mFrameLayout=" + (mFrameLayout != null ? "OK" : "NULL"));
        if (mFrameLayout == null) {
            return;
        }
        FrameLayout container = new FrameLayout(this);
        container.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        container.setVisibility(View.VISIBLE);
        // The container must not swallow touches: it hosts the Java screens, the
        // GLSurfaceView below keeps receiving input for the world/touch pads.
        container.setClickable(false);
        container.setFocusable(false);
        mFrameLayout.addView(container);
        mJavaUi = new JavaUiHost(this, container);
        SurvBridge.setHost(mJavaUi);
        mJavaUi.install();
        Log.i(TAG, "JavaUiHost installed, java menu should be visible");
    }

}
