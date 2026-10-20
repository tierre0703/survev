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

    private SurvApi() {}

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

    // --- Simple callback interface (no lambdas, Java 8 compatible) ----------

    public interface Callback {
        void onResult(String json);
    }

    // --- HTTP helpers -------------------------------------------------------

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
            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(conn.getInputStream(), "UTF-8"));
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

    private static String escapeJson(String s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                default: out.append(c); break;
            }
        }
        return out.toString();
    }
}