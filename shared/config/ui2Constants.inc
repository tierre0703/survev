// Constants shared by the web client's HUD (client/src/ui/ui2.ts) and the
// native port (client-mobile/src/ui/FguiHud.cpp).
//
// The CSS in client/css/game.css is authored in CSS pixels; ui2.ts divides
// every size by `UI2.DEVICE_SCALE` (devicePixelRatio rounded to 1/1.5/2) so the
// HUD keeps a constant physical size across phones. The native HUD lives in the
// 1280x720 design space, and `ui::layoutUiRoot()` scales that root by
// `min(visible / design)` points, which is the same points-per-CSS-pixel factor
// on the Android target (px == points there). Carrying the constant here keeps
// both HUDs identical.
#ifndef SURV_SHARED_UI2_CONSTANTS_INC
#define SURV_SHARED_UI2_CONSTANTS_INC

namespace ui2 {
inline constexpr float kDeviceScale = 1.5f;
} // namespace ui2

#endif
