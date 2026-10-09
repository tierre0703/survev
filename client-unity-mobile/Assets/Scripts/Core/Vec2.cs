// Port of shared/utils/v2.ts — 2D vector helpers shared with the server.
using System;

namespace Survev.Core
{
    /// <summary>Port of the subset of <c>shared/utils/v2.ts</c> the client uses.</summary>
    public struct Vec2 : IEquatable<Vec2>
    {
        public float X;
        public float Y;

        public Vec2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public static readonly Vec2 Zero = new Vec2(0f, 0f);
        public static readonly Vec2 One = new Vec2(1f, 1f);

        public static Vec2 Add(Vec2 a, Vec2 b) => new Vec2(a.X + b.X, a.Y + b.Y);
        public static Vec2 Sub(Vec2 a, Vec2 b) => new Vec2(a.X - b.X, a.Y - b.Y);
        public static Vec2 Mul(Vec2 a, float s) => new Vec2(a.X * s, a.Y * s);
        public static Vec2 Div(Vec2 a, float s) => new Vec2(a.X / s, a.Y / s);

        public static float Dot(Vec2 a, Vec2 b) => a.X * b.X + a.Y * b.Y;

        public static float Length(Vec2 v) => (float)Math.Sqrt(v.X * v.X + v.Y * v.Y);
        public static float LengthSqr(Vec2 v) => v.X * v.X + v.Y * v.Y;

        public static float Distance(Vec2 a, Vec2 b) => Length(Sub(a, b));
        public static float DistanceSqr(Vec2 a, Vec2 b) => LengthSqr(Sub(a, b));

        public static Vec2 Normalize(Vec2 v)
        {
            float len = Length(v);
            return len > 0f ? Div(v, len) : Zero;
        }

        public static Vec2 Lerp(Vec2 a, Vec2 b, float t) => new Vec2(
            a.X + (b.X - a.X) * t,
            a.Y + (b.Y - a.Y) * t
        );

        public float Length() => Length(this);
        public float LengthSqr() => LengthSqr(this);
        public Vec2 Normalized() => Normalize(this);

        public bool Equals(Vec2 other) => X == other.X && Y == other.Y;
        public override bool Equals(object obj) => obj is Vec2 other && Equals(other);
        public override int GetHashCode() => X.GetHashCode() ^ (Y.GetHashCode() << 2);

        public static Vec2 operator +(Vec2 a, Vec2 b) => Add(a, b);
        public static Vec2 operator -(Vec2 a, Vec2 b) => Sub(a, b);
        public static Vec2 operator *(Vec2 a, float s) => Mul(a, s);
        public static Vec2 operator /(Vec2 a, float s) => Div(a, s);

        public override string ToString() => $"({X}, {Y})";
    }
}