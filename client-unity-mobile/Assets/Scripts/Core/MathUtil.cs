// Port of shared/utils/math.ts — the small numeric helpers the client and the
// shared simulation rely on. Must stay behaviourally identical (clamp/lerp/rand).
using System;

namespace Survev.Core
{
    /// <summary>Port of the subset of <c>shared/utils/math.ts</c> the client uses.</summary>
    public static class MathUtil
    {
        public static float Clamp(float value, float min, float max)
        {
            if (value < min)
            {
                return min;
            }
            return value > max ? max : value;
        }

        public static int Clamp(int value, int min, int max)
        {
            if (value < min)
            {
                return min;
            }
            return value > max ? max : value;
        }

        public static float Clamp01(float value) => Clamp(value, 0f, 1f);

        public static float Lerp(float a, float b, float t) => a + (b - a) * t;

        public static float InverseLerp(float a, float b, float value)
        {
            return a != b ? Clamp01((value - a) / (b - a)) : 0f;
        }

        public static float Map(float value, float inMin, float inMax, float outMin, float outMax)
        {
            return Lerp(outMin, outMax, InverseLerp(inMin, inMax, value));
        }

        public static float RadToDeg(float radians) => radians * (180f / (float)Math.PI);
        public static float DegToRad(float degrees) => degrees * ((float)Math.PI / 180f);

        /// <summary>Smallest signed angle from <paramref name="from"/> to <paramref name="to"/>.</summary>
        public static float GetAngleDist(float from, float to)
        {
            float dist = (to - from) % ((float)Math.PI * 2f);
            dist = ((2f * dist) % ((float)Math.PI * 2f)) - dist;
            return dist;
        }

        public static float ClampAngle(float angle) => WrapAngle(angle);

        public static float WrapAngle(float angle)
        {
            angle = (float)((angle + Math.PI) % (Math.PI * 2.0));
            if (angle < 0f)
            {
                angle += (float)(Math.PI * 2.0);
            }
            return angle - (float)Math.PI;
        }

        // Deterministic RNG helpers (NOT System.Random) — the sim must be
        // reproducible across clients. See Rand.cs for the port of the TS PRNG.
        public static float RandomFloat(Rand rng) => rng.NextFloat();
        public static int RandomInt(Rand rng, int minInclusive, int maxExclusive)
            => rng.NextInt(minInclusive, maxExclusive);
    }
}