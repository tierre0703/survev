// Port of client/src/camera.ts — the world <-> screen transform.
//
// The web client keeps a single camera with a scale (ppu/zoom), a point offset
// and screen shake. The same fields must survive so gameplay coordinates render
// identically on Android. See plan.md M4.
using Survev.Core;

namespace Survev.Render
{
    /// <summary>Port placeholder for <c>client/src/camera.ts</c>.</summary>
    public sealed class Camera
    {
        public Vec2 Point = Vec2.Zero;

        /// <summary>Pixels per world unit (web client's zoom).</summary>
        public float Scale = 1f;

        /// <summary>Design-space (CSS) viewport size, e.g. 1280x720 landscape.</summary>
        public Vec2 Viewport = new Vec2(1280f, 720f);

        public Vec2 PointToScreen(Vec2 world)
        {
            return new Vec2(
                (world.X - Point.X) * Scale + Viewport.X * 0.5f,
                (world.Y - Point.Y) * Scale + Viewport.Y * 0.5f
            );
        }

        public Vec2 ScreenToPoint(Vec2 screen)
        {
            return new Vec2(
                (screen.X - Viewport.X * 0.5f) / Scale + Point.X,
                (screen.Y - Viewport.Y * 0.5f) / Scale + Point.Y
            );
        }
    }
}