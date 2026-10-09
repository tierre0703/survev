// Port of client/src/api.ts + client/src/siteInfo.ts:
//   find_game_v2, site_info, region list and ping targets.
//
// Uses UnityWebRequest (works on Android without a browser). The request/response
// shapes are the shared types in shared/types/api.ts, so keep field names and the
// endpoints byte-identical to the web client.
using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Networking;

namespace Survev.Net
{
    /// <summary>Port placeholder for the web client's HTTP API helper.</summary>
    public static class Api
    {
        // Matches DevConfig on the web/axmol clients: emulator host loopback is
        // 10.0.2.2, and `adb reverse tcp:8000 tcp:8000` maps it to 127.0.0.1.
        public const string DefaultBaseUrl = "http://10.0.2.2:8000";

        public static IEnumerator GetJson(string url, Action<string> onDone, Action<string> onError)
        {
            using (var request = UnityWebRequest.Get(url))
            {
                yield return request.SendWebRequest();
#if UNITY_2020_1_OR_NEWER
                if (request.result == UnityWebRequest.Result.Success)
#else
                if (!request.isNetworkError && !request.isHttpError)
#endif
                {
                    onDone?.Invoke(request.downloadHandler.text);
                }
                else
                {
                    onError?.Invoke(request.error);
                }
            }
        }

        public static string FindGameUrl(string baseUrl) => baseUrl.TrimEnd('/') + "/api/find_game_v2";
        public static string SiteInfoUrl(string baseUrl) => baseUrl.TrimEnd('/') + "/api/site_info";
    }
}