// Port of client/src/ui/localization.ts (the JSON-loading part) plus the
// StreamingAssets file access the rest of the UI uses.
//
// Localization.cs holds the registry; this loader reads the bundled
// `l10n/<locale>.json` (English is `en.json`) and installs it. The keys are
// identical to the web client.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.Networking;

namespace Survev.Config
{
    /// <summary>Loads bundled localization + assets from StreamingAssets.</summary>
    public static class LocalizationLoader
    {
        /// <summary>
        /// Load the English fallback synchronously (always available) then the
        /// requested locale. Returns a coroutine so the caller can await it.
        /// </summary>
        public static IEnumerator Load(string locale)
        {
            yield return LoadEnglish();
            if (!string.IsNullOrEmpty(locale) && locale != Localization.DefaultLocale)
            {
                Dictionary<string, string> strings = null;
                yield return ReadLocale(locale, value => strings = value);
                Localization.SetLocale(locale, strings ?? new Dictionary<string, string>());
            }
            else
            {
                Localization.SetLocale(Localization.DefaultLocale, null);
            }
        }

        private static IEnumerator LoadEnglish()
        {
            Dictionary<string, string> english = null;
            yield return ReadLocale(Localization.DefaultLocale, value => english = value);
            Localization.RegisterEnglish(english ?? new Dictionary<string, string>());
        }

        private static IEnumerator ReadLocale(string locale, Action<Dictionary<string, string>> onDone)
        {
            string path = Path.Combine(Application.streamingAssetsPath, "l10n", locale + ".json");
            string url = path.Contains("://") ? path : "file://" + path;

            using (var request = UnityWebRequest.Get(url))
            {
                yield return request.SendWebRequest();
#if UNITY_2020_1_OR_NEWER
                bool ok = request.result == UnityWebRequest.Result.Success;
#else
                bool ok = !request.isNetworkError && !request.isHttpError;
#endif
                if (ok)
                {
                    onDone(ParseFlat(request.downloadHandler.text));
                }
                else
                {
                    onDone(null);
                }
            }
        }

        /// <summary>Parse a flat JSON object of string values (the l10n format).</summary>
        public static Dictionary<string, string> ParseFlat(string json)
        {
            var result = new Dictionary<string, string>(StringComparer.Ordinal);
            if (string.IsNullOrEmpty(json))
            {
                return result;
            }

            // Minimal flat-object parser (no nested values in l10n files). A full
            // JSON reader is available through Newtonsoft.Json in ThirdParty if
            // the format ever grows.
            int i = 0;
            SkipWs(json, ref i);
            if (i >= json.Length || json[i] != '{')
            {
                return result;
            }
            i++;
            while (i < json.Length)
            {
                SkipWs(json, ref i);
                if (i < json.Length && json[i] == '}')
                {
                    break;
                }
                string key = ReadJsonString(json, ref i);
                SkipWs(json, ref i);
                if (i < json.Length && json[i] == ':')
                {
                    i++;
                }
                SkipWs(json, ref i);
                string value = ReadJsonString(json, ref i);
                if (key != null && value != null)
                {
                    result[key] = value;
                }
                SkipWs(json, ref i);
                if (i < json.Length && json[i] == ',')
                {
                    i++;
                }
            }
            return result;
        }

        private static void SkipWs(string s, ref int i)
        {
            while (i < s.Length && char.IsWhiteSpace(s[i]))
            {
                i++;
            }
        }

        private static string ReadJsonString(string s, ref int i)
        {
            if (i >= s.Length || s[i] != '"')
            {
                return null;
            }
            i++;
            var sb = new System.Text.StringBuilder();
            while (i < s.Length && s[i] != '"')
            {
                if (s[i] == '\\' && i + 1 < s.Length)
                {
                    i++;
                    switch (s[i])
                    {
                        case 'n': sb.Append('\n'); break;
                        case 't': sb.Append('\t'); break;
                        case 'r': sb.Append('\r'); break;
                        case '"': sb.Append('"'); break;
                        case '\\': sb.Append('\\'); break;
                        case '/': sb.Append('/'); break;
                        case 'u':
                            if (i + 4 < s.Length &&
                                int.TryParse(s.Substring(i + 1, 4),
                                    System.Globalization.NumberStyles.HexNumber,
                                    System.Globalization.CultureInfo.InvariantCulture, out int code))
                            {
                                sb.Append((char)code);
                                i += 4;
                            }
                            break;
                        default: sb.Append(s[i]); break;
                    }
                }
                else
                {
                    sb.Append(s[i]);
                }
                i++;
            }
            i++; // closing quote
            return sb.ToString();
        }
    }
}
