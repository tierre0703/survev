// Port of the touch half of client/src/ui/touch.ts — the dual joystick.
//
// The web client uses two on-screen pads: one for movement, one for aim/fire.
// Touch positions are in CSS/screen space, so the same math (dead-zone,
// locked/anywhere styles, throwable latch, turn-to-move aim cooldown) is reused
// on Android. Keep the numbers identical; see plan.md M5.
using Survev.Core;

namespace Survev.UI
{
    public enum TouchStyle
    {
        Locked = 0,
        Anywhere = 1,
    }

    /// <summary>Port placeholder for <c>client/src/ui/touch.ts</c>.</summary>
    public sealed class Touch
    {
        public const float DeadZone = 2f;
        public const float SensitivityThreshold = 0.00001f;

        public bool Active { get; private set; }
        public Vec2 PosDown { get; private set; }
        public Vec2 Pos { get; private set; }
        public TouchStyle Style { get; set; } = TouchStyle.Locked;

        public void OnDown(Vec2 screenPos, float screenScale)
        {
            Active = true;
            PosDown = screenPos / screenScale;
            Pos = PosDown;
        }

        public void OnMove(Vec2 screenPos, float screenScale)
        {
            if (!Active)
            {
                return;
            }
            Pos = screenPos / screenScale;
        }

        public void OnUp()
        {
            Active = false;
        }
    }
}