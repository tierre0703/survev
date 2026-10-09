// Port of shared/utils/loadout.ts — the player's selected items.
//
// A 1:1 port is required because the JoinMsg serializes the loadout, so the
// indices here must match the server's expectations exactly. This stub keeps the
// project compiling before the port lands (see plan.md M2/M8).
using System.Collections.Generic;

namespace Survev.Core
{
    /// <summary>Port placeholder for <c>shared/utils/loadout.ts</c>.</summary>
    public sealed class Loadout
    {
        private readonly Dictionary<string, object> _values = new Dictionary<string, object>();

        public object Get(string key)
        {
            return _values.TryGetValue(key, out var value) ? value : null;
        }

        public void Set(string key, object value)
        {
            _values[key] = value;
        }
    }
}