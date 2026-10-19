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
import android.os.Bundle;
import android.os.Build;
import dev.axmol.lib.AxmolActivity;
import dev.axmol.lib.SharedLoader;
import android.view.KeyEvent;
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

public class AppActivity extends AxmolActivity {
    private static final String TAG = "AppActivity";

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
        super.setEnableVirtualButton(false);
        super.onCreate(savedInstanceState);
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
    }

    @Override
    protected void onDestroy() {
        if (sCurrent == this) {
            sCurrent = null;
        }
        super.onDestroy();
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
