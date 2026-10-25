/****************************************************************************
 * SurvApi.java — HTTP client for the Survev API (S3).
 *
 * Uses plain java.net.HttpURLConnection (no external dependencies) so the
 * Gradle-free APK pipeline needs no vendored jars (ARCHITECTURE.md §6.2).
 *
 * Two endpoints:
 *   GET  /api/site_info   → site info (regions, modes, server status)
 *   POST /api/find_game_v2 → matchmaking (returns join URLs + token)
 *
 * Threading rules (ARCHITECTURE.md §4.2):
 *   * Network calls run on a background thread (new Thread).
 *   * Results are delivered via SurvBridge callbacks on the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.util.Log;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public final class SurvApi {
    private static final String TAG = "SurvApi";
    private static final int TIMEOUT_MS = 15000;

    /** Default API origin (mirrors the C++ DevConfig.h `kApiBaseUrl`). */
    public static final String DEFAULT_API_BASE_URL = "http://127.0.0.1:8000";

    private SurvApi() {}

    /**
     * Synchronously POSTs `/api/report_error` (client/src/errorLogs.ts). Used by
     * the crash handler, where the process may die before a queued report could
     * run; call it off the UI thread.
     */
    public static void reportError(String apiBaseUrl, String loc, String dataJson) {
        String body = "{\"loc\":\"" + escapeJson(loc) + "\",\"data\":"
                + (dataJson == null || dataJson.isEmpty() ? "null" : dataJson) + "}";
        httpPost(apiBaseUrl + "/api/report_error", body);
    }

    /**
     * Fetches GET /api/site_info and calls `callback` with the JSON response
     * text on the UI thread. Calls callback(null) on error.
     */
    public static void siteInfo(final String apiBaseUrl, final Callback callback) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "siteInfo: starting HTTP GET");
                final String result = httpGet(apiBaseUrl + "/api/site_info");
                Log.i(TAG, "siteInfo: got result=" + (result != null ? result.substring(0, Math.min(result.length(), 80)) : "null"));
                AppActivity activity = AppActivity.current();
                if (activity != null) {
                    final String r = result;
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            callback.onResult(r);
                        }
                    });
                }
            }
        }).start();
    }

    /**
     * POSTs a find_game request and calls `callback` with the JSON response
     * text on the UI thread. The body is the JSON payload matching FindGameBody.
     * Calls callback(null) on error.
     */
    public static void findGame(final String apiBaseUrl, final String requestBody,
                                 final Callback callback) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                final String result = httpPost(apiBaseUrl + "/api/find_game_v2",
                                               requestBody);
                AppActivity activity = AppActivity.current();
                if (activity != null) {
                    final String r = result;
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            callback.onResult(r);
                        }
                    });
                }
            }
        }).start();
    }

    /**
     * Creates a new team and returns teamId + inviteCode.
     */
    public static void createTeam(final String apiBaseUrl, final Callback callback) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                String body = "{}";
                final String result = httpPost(apiBaseUrl + "/api/team/create", body);
                AppActivity activity = AppActivity.current();
                if (activity != null) {
                    final String r = result;
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            callback.onResult(r);
                        }
                    });
                }
            }
        }).start();
    }

    /**
     * Joins an existing team by invite code.
     */
    public static void joinTeam(final String apiBaseUrl, final String inviteCode,
                                final Callback callback) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                String body = "{\"inviteCode\":\"" + escapeJson(inviteCode) + "\"}";
                final String result = httpPost(apiBaseUrl + "/api/team/join", body);
                AppActivity activity = AppActivity.current();
                if (activity != null) {
                    final String r = result;
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            callback.onResult(r);
                        }
                    });
                }
            }
        }).start();
    }

    /**
     * M8 device login (no OAuth): POSTs the persisted install id and receives a
     * session token for `userPost`.
     */
    public static void deviceLogin(final String apiBaseUrl, final String deviceId,
                                   final StatusCallback callback) {
        String body = "{\"deviceId\":\"" + escapeJson(deviceId) + "\"}";
        postAsync(apiBaseUrl + "/api/auth/device", body, null, callback);
    }

    /**
     * Calls an authenticated `/api/user/<action>` endpoint. `token` is the
     * device-login session token; it is sent as `Authorization: Bearer` because
     * the native client has no cookie jar (server: authMiddleware).
     */
    public static void userPost(final String apiBaseUrl, final String action,
                                final String token, final String requestBody,
                                final StatusCallback callback) {
        postAsync(apiBaseUrl + "/api/user/" + action, requestBody, token, callback);
    }

    // --- Simple callback interface (no lambdas, Java 8 compatible) ----------

    public interface Callback {
        void onResult(String json);
    }

    /**
     * Status-aware callback for the M8 account calls: `httpCode` is the HTTP
     * status (or -1 on a transport error) and `json` the response body.
     */
    public interface StatusCallback {
        void onResult(int httpCode, String json);
    }

    // --- HTTP helpers -------------------------------------------------------

    private static void postAsync(final String urlString, final String body,
                                  final String token, final StatusCallback callback) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                final HttpResult result = httpPostWithToken(urlString, body, token);
                AppActivity activity = AppActivity.current();
                if (activity != null) {
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            callback.onResult(result.code, result.body);
                        }
                    });
                }
            }
        }).start();
    }

    private static final class HttpResult {
        int code = -1;
        String body = null;
    }

    private static HttpResult httpPostWithToken(String urlString, String body, String token) {
        HttpResult result = new HttpResult();
        try {
            URL url = new URL(urlString);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setConnectTimeout(TIMEOUT_MS);
            conn.setReadTimeout(TIMEOUT_MS);
            conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
            if (token != null && !token.isEmpty()) {
                conn.setRequestProperty("Authorization", "Bearer " + token);
            }
            conn.setDoOutput(true);
            OutputStreamWriter writer = new OutputStreamWriter(conn.getOutputStream(), "UTF-8");
            writer.write(body);
            writer.close();
            int code = conn.getResponseCode();
            result.code = code;
            result.body = readStream(code >= 200 && code < 300
                    ? conn.getInputStream()
                    : conn.getErrorStream());
            if (code < 200 || code >= 300) {
                Log.w(TAG, "POST " + urlString + " -> " + code + " " + result.body);
            }
        } catch (Exception e) {
            Log.w(TAG, "POST " + urlString + " failed: " + e.getMessage());
        }
        return result;
    }

    private static String httpGet(String urlString) {
        try {
            URL url = new URL(urlString);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(TIMEOUT_MS);
            conn.setReadTimeout(TIMEOUT_MS);
            int code = conn.getResponseCode();
            if (code >= 200 && code < 300) {
                return readStream(conn);
            }
            Log.w(TAG, "GET " + urlString + " -> " + code);
            return null;
        } catch (Exception e) {
            Log.w(TAG, "GET " + urlString + " failed: " + e.getMessage());
            return null;
        }
    }

    private static String httpPost(String urlString, String body) {
        try {
            URL url = new URL(urlString);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setConnectTimeout(TIMEOUT_MS);
            conn.setReadTimeout(TIMEOUT_MS);
            conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
            conn.setDoOutput(true);
            OutputStreamWriter writer = new OutputStreamWriter(conn.getOutputStream(), "UTF-8");
            writer.write(body);
            writer.close();
            int code = conn.getResponseCode();
            if (code >= 200 && code < 300) {
                return readStream(conn);
            }
            Log.w(TAG, "POST " + urlString + " -> " + code);
            return null;
        } catch (Exception e) {
            Log.w(TAG, "POST " + urlString + " failed: " + e.getMessage());
            return null;
        }
    }

    private static String readStream(HttpURLConnection conn) {
        try {
            return readStream(conn.getInputStream());
        } catch (Exception e) {
            Log.w(TAG, "read stream failed: " + e.getMessage());
            return null;
        }
    }

    private static String readStream(java.io.InputStream stream) {
        if (stream == null) return null;
        try {
            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(stream, "UTF-8"));
            StringBuilder out = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                out.append(line).append('\n');
            }
            reader.close();
            return out.toString();
        } catch (Exception e) {
            Log.w(TAG, "read stream failed: " + e.getMessage());
            return null;
        }
    }

    public static String escapeJson(String s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                case '\t': out.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        out.append(String.format("\\u%04x", (int) c));
                    } else {
                        out.append(c);
                    }
                    break;
            }
        }
        return out.toString();
    }
}