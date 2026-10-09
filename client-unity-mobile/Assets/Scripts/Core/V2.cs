// Port of shared/utils/v2.ts being used as wire helpers is not enough for the
// mobile client; this small class mirrors the `v2` namespace the web code calls
// (v2.create, v2.add, v2.sub, v2.len, ...). Kept separate from Core.Vec2 so the
// ported math reads like the TypeScript.
using System;

namespace Survev.Core
{
    /// <summary>Port of the <c>v2</c> namespace in shared/utils/v2.ts.</summary>
    public static class v2
    {
        public static Vec2 Create(float x = 0f, float y = 0f) => new Vec2(x, y);

        public static Vec2 Add(Vec2 a, Vec2 b) => new Vec2(a.X + b.X, a.Y + b.Y);
        public static Vec2 Sub(Vec2 a, Vec2 b) => new Vec2(a.X - b.X, a.Y - b.Y);
        public static Vec2 Mul(Vec2 a, float s) => new Vec2(a.X * s, a.Y * s);
        public static Vec2 Div(Vec2 a, float s) => new Vec2(a.X / s, a.Y / s);

        public static float Len(Vec2 a) => (float)Math.Sqrt(a.X * a.X + a.Y * a.Y);
        public static float LenSqr(Vec2 a) => a.X * a.X + a.Y * a.Y;

        public static float Dot(Vec2 a, Vec2 b) => a.X * b.X + a.Y * b.Y;

        public static Vec2 Normalize(Vec2 a)
        {
            float len = Len(a);
            return len > 0f ? Div(a, len) : Zero;
        }

        public static float Distance(Vec2 a, Vec2 b) => Len(Sub(a, b));

        public static Vec2 Lerp(Vec2 a, Vec2 b, float t) => new Vec2(
            a.X + (b.X - a.X) * t,
            a.Y + (b.Y - a.Y) * t
        );

        public static float Angle(Vec2 a) => (float)Math.Atan2(a.Y, a.X);
        public static Vec2 FromAngle(float angle) => new Vec2((float)Math.Cos(angle), (float)Math.Sin(angle));

        public static Vec2 Zero => Vec2.Zero;
    }
}
