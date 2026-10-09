// Port of client/src/ui/localization.ts — 18 locales loaded from
// Assets/StreamingAssets/l10n/*.json (English falls back to en.json).
//
// The keys are identical to the web client (client/src/en.json). The loader is
// async (UnityWebRequest/StreamingAssets) so the menu can build before the
// active locale resolves, exactly like the web client's lazy load.
using System;
using System.Collections.Generic;
using UnityEngine;

namespace Survev.Config
{
    /// <summary>Localization registry + JSON loader.</summary>
    public static class Localization
    {
        public const string DefaultLocale = "en";

        public static readonly string[] Locales =
        {
            "da", "de", "en", "es", "fr", "it", "jp", "nl", "pl", "pt",
            "ru", "sv", "th", "tr", "vn", "zh-cn", "zh-tw",
        };

        private static readonly Dictionary<string, string> _strings =
            new Dictionary<string, string>(StringComparer.Ordinal);

        private static readonly Dictionary<string, string> _english =
            new Dictionary<string, string>(StringComparer.Ordinal);

        public static string Locale { get; private set; } = DefaultLocale;

        /// <summary>Register the English map used as the fallback for every locale.</summary>
        public static void RegisterEnglish(Dictionary<string, string> english)
        {
            _english.Clear();
            if (english == null)
            {
                return;
            }
            foreach (var kv in english)
            {
                _english[kv.Key] = kv.Value;
            }
        }

        /// <summary>Switch locale; falls back to English for missing keys.</summary>
        public static void SetLocale(string locale, Dictionary<string, string> strings)
        {
            Locale = string.IsNullOrEmpty(locale) ? DefaultLocale : locale.ToLowerInvariant();
            _strings.Clear();
            if (strings != null)
            {
                foreach (var kv in strings)
                {
                    _strings[kv.Key] = kv.Value;
                }
            }
        }

        public static string Translate(string key)
        {
            if (string.IsNullOrEmpty(key))
            {
                return string.Empty;
            }
            if (_strings.TryGetValue(key, out var value))
            {
                return value;
            }
            return _english.TryGetValue(key, out var fallback) ? fallback : key;
        }

        /// <summary>Pick the best locale for the device language.</summary>
        public static string DetectLocale()
        {
            string device = Application.systemLanguage.ToString().ToLowerInvariant();
            foreach (var locale in Locales)
            {
                if (device.StartsWith(locale, StringComparison.Ordinal))
                {
                    return locale;
                }
            }
            return DefaultLocale;
        }
    }
}