// Port of shared/utils/coldet.ts collider types (the subset the net protocol
// and the client simulation use).
//
// The wire format (writeCollider/readCollider in shared/net/net.ts) depends on
// `Type` being 0 for AABB and 1 for Circle, and on the field layout — keep it.
using Survev.Core;

namespace Survev.Net
{
    public enum ColliderType
    {
        Aabb = 0,
        Circle = 1,
    }

    /// <summary>Port of the <c>Collider</c> shape used by coldet.ts.</summary>
    public struct Collider
    {
        public ColliderType Type;
        public Vec2 Min;
        public Vec2 Max;
        public Vec2 Pos;
        public float Rad;

        public static Collider CreateAabb(Vec2 min, Vec2 max)
        {
            return new Collider
            {
                Type = ColliderType.Aabb,
                Min = min,
                Max = max,
                Pos = (min + max) * 0.5f,
                Rad = 0f,
            };
        }

        public static Collider CreateCircle(Vec2 pos, float rad)
        {
            return new Collider
            {
                Type = ColliderType.Circle,
                Pos = pos,
                Rad = rad,
                Min = pos - new Vec2(rad, rad),
                Max = pos + new Vec2(rad, rad),
            };
        }
    }
}
