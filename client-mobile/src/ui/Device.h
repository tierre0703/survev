#pragma once
// Port of the parts of client/src/device.ts the native UI depends on, plus the
// resources.ts texture-resolution rule (`selectTextureRes`).
//
// The native app is always an Android touch device, so the web client's
// UA-based `os`/`mobile` detection becomes constants. `tablet` cannot come from
// a user agent in a native app (and Android tablets reported desktop-like UAs
// to isMobile anyway), so it is classified from the physical screen: the
// web's tablet cut-off, a >= 7" diagonal. `uiLayout` mirrors device.onResize()
// exactly.
#include <cmath>
#include <string>

namespace ui {

class Device {
public:
    enum Layout { Lg = 0, Sm = 1 };

    // resources.ts `spritesheetDefs[textureRes]` sheet resolution.
    enum class TextureRes { High, Low };

    // device.ts `tablet` approximation: physical screen diagonal cut-off
    // (isMobile's tablet viewport bucket is 7..13").
    static constexpr float kTabletDiagonalInches = 7.0f;

    // Android is the only target; iOS would be the other mobile OS.
    const std::string os = "android";
    const bool mobile = true;
    const bool touch = true;

    // device.ts `tablet` (isMobile.tablet / isIpad()) — set by `resize` from
    // the physical screen when a DPI was supplied; `false` until then.
    // `dpi` is Android physical dots per inch (DisplayMetrics.xdpi) and
    // `pixelRatio` mirrors window.devicePixelRatio (dpi / 160).
    bool tablet = false;
    int dpi = 160;
    float pixelRatio = 1.0f;

    // Re-derived in `resize` (mirrors device.onResize()).
    bool isLandscape = true;
    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    Layout uiLayout = Sm;

    static Device& get();

    // device.ts onResize(). `width`/`height` are screen pixels; when `dpi` is
    // > 0 the tablet flag is (re)classified from the physical screen.
    void resize(float width, float height, float pixelRatio_ = 1.0f, int dpi_ = 0) {
        screenWidth = width;
        screenHeight = height;
        if (pixelRatio_ > 0.0f) {
            pixelRatio = pixelRatio_;
        }
        if (dpi_ > 0) {
            dpi = dpi_;
            // Only a call with a physical DPI (the app's GL view) classifies
            // the tablet flag; earlier callers (e.g. the desktop menu scene,
            // which passes design units) keep the previous value.
            tablet = classifyTablet(width, height, dpi);
        }
        isLandscape = width > height;
        const float layoutDim = isLandscape ? screenWidth : screenHeight;
        uiLayout = (mobile || layoutDim <= 850.0f || (layoutDim <= 900.0f && pixelRatio >= 3.0f))
                       ? Sm
                       : Lg;
    }

    // device.ts `tablet` for the native client: `widthPx`/`heightPx` are the
    // physical screen pixels and `dpi` the physical density. Zero/None inputs
    // never classify as a tablet (the web's UA detection never guesses either).
    static bool classifyTablet(float widthPx, float heightPx, float dpi) {
        if (dpi <= 0.0f || widthPx <= 0.0f || heightPx <= 0.0f) {
            return false;
        }
        const float diagonal = std::sqrt(widthPx * widthPx + heightPx * heightPx) /
            static_cast<float>(dpi);
        return diagonal >= kTabletDiagonalInches;
    }

    bool isSmallLayout() const { return uiLayout == Sm; }

    // resources.ts selectTextureRes(). `screenW`/`screenH` are CSS screen
    // pixels (window.screen) and `maxTextureSize` the GL driver's value. Reads
    // this instance's mobile/tablet flags like the web reads the singleton.
    //
    // Note: the native app only ships the low-res sheets
    // (`tools/build-atlas.mjs --res low`) and `highResTex` was replaced by the
    // EGL surface GFX tiers (REMAININGS §3.5), so the loader currently always
    // loads the low-res sheets; this port records which tier the web client
    // would pick for the same device.
    TextureRes selectTextureRes(bool highResTex, unsigned int maxTextureSize, float screenW,
                                float screenH, float pixelRatio_) const {
        float minDim = screenW < screenH ? screenW : screenH;
        float maxDim = screenW < screenH ? screenH : screenW;
        minDim *= pixelRatio_;
        maxDim *= pixelRatio_;
        const bool smallScreen = maxDim < 1366.0f && minDim < 768.0f;
        TextureRes textureRes = highResTex ? TextureRes::High : TextureRes::Low;
        if (smallScreen || (mobile && !tablet) || maxTextureSize < 4096u) {
            textureRes = TextureRes::Low;
        }
        return textureRes;
    }
};

} // namespace ui
