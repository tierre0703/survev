// Port of shared/utils/Rand.ts (the deterministic PRNG used by the sim and the
// map generator). The web client seeds it and expects reproducible sequences, so
// the algorithm (not System.Random) must be reproduced exactly.
//
// This stub documents the contract; the full xorshift/mulberry32 port lands with
// the M2 core work (see plan.md).
using System;

namespace Survev.Core
{
    /// <summary>Port placeholder for the shared deterministic RNG.</summary>
    public sealed class Rand
    {
        private uint _state;

        public Rand(uint seed = 0u)
        {
            _state = seed;
        }

        /// <summary>mulberry32, matching the shared TS PRNG.</summary>
        public uint NextUInt32()
        {
            _state += 0x6D2B79F5u;
            uint t = _state;
            t = (uint)((t ^ (t >> 15)) * (t | 1u));
            t ^= t + (uint)((t ^ (t >> 7)) * (t | 61u));
            return t ^ (t >> 14);
        }

        /// <summary>[0, 1) float, as the web client's random float.</summary>
        public float NextFloat()
        {
            return NextUInt32() / 4294967296f;
        }

        public int NextInt(int minInclusive, int maxExclusive)
        {
            if (maxExclusive <= minInclusive)
            {
                return minInclusive;
            }
            uint range = (uint)(maxExclusive - minInclusive);
            return minInclusive + (int)(NextUInt32() % range);
        }

        public float Range(float min, float max)
        {
            return min + NextFloat() * (max - min);
        }
    }
}
