/****************************************************************************
 * TeamSocket.java — WebSocket client for the /team_v2 JSON control channel
 * (ARCHITECTURE.md §5.2 / A3).
 *
 * Uses java.net.HttpURLConnection for the upgrade handshake (no external WS
 * library) and a background thread for the read loop.
 *
 * Protocol: JSON messages over a plain WebSocket.
 *   Client -> Server: {"type":"create"/"join"/"setRoomProps"/"playGame"/"keepAlive"/"gameComplete"}
 *   Server -> Client: {"type":"state"/"joinGame"/"kicked"/"error"}
 *
 * Threading: all C++ bridge calls (SurvBridge.onJoinUrls) happen on the UI
 * thread; socket reads happen on a background thread.
 ****************************************************************************/
package dev.axmol.app;

import android.util.Log;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;

public class TeamSocket {
    private static final String TAG = "TeamSocket";
    private static final int TIMEOUT_MS = 10000;

    private String mUrl;
    private Thread mReaderThread;
    private volatile boolean mRunning = false;
    private InputStream mInputStream;
    private OutputStream mOutputStream;

    // --- callbacks (called from reader thread) ---
    private OnMessage mOnMessage;
    private OnError mOnError;

    public interface OnMessage {
        void onMessage(String json);
    }

    public interface OnError {
        void onError(String reason);
    }

    public TeamSocket() {}

    /**
     * Opens a WebSocket connection to the /team_v2 endpoint.
     * url format: "ws://host:port/team_v2" or "wss://host:port/team_v2"
     */
    public boolean connect(String url, OnMessage onMessage, OnError onError) {
        mUrl = url;
        mOnMessage = onMessage;
        mOnError = onError;

        try {
            // Convert ws:// to http:// for the upgrade
            String httpUrl = url.replace("ws://", "http://").replace("wss://", "https://");
            URL upgradeUrl = new URL(httpUrl);
            HttpURLConnection conn = (HttpURLConnection) upgradeUrl.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(TIMEOUT_MS);
            conn.setReadTimeout(TIMEOUT_MS);

            // WebSocket upgrade headers
            String key = generateKey();
            conn.setRequestProperty("Upgrade", "websocket");
            conn.setRequestProperty("Connection", "Upgrade");
            conn.setRequestProperty("Sec-WebSocket-Key", key);
            conn.setRequestProperty("Sec-WebSocket-Version", "13");
            conn.setRequestProperty("Origin", "http://survev.mobile");

            int code = conn.getResponseCode();
            String upgrade = conn.getHeaderField("Upgrade");
            String accept = conn.getHeaderField("Sec-WebSocket-Accept");

            if (code != 101 || upgrade == null || !upgrade.toLowerCase().contains("websocket")) {
                if (onError != null) onError.onError("WebSocket upgrade failed: " + code);
                return false;
            }

            mInputStream = conn.getInputStream();
            mOutputStream = conn.getOutputStream();
            mRunning = true;

            // Start reader thread
            mReaderThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    readLoop();
                }
            });
            mReaderThread.start();

            return true;
        } catch (Exception e) {
            Log.w(TAG, "connect failed: " + e.getMessage());
            if (onError != null) onError.onError(e.getMessage());
            return false;
        }
    }

    /** Sends a JSON text frame. */
    public void send(String text) {
        if (!mRunning || mOutputStream == null) return;
        try {
            byte[] data = text.getBytes("UTF-8");
            // Text frame: FIN=1, opcode=0x1, mask=1
            byte[] header;
            if (data.length < 126) {
                header = new byte[6]; // 2 header + 4 mask
                header[0] = (byte) 0x81; // FIN + text
                header[1] = (byte) (0x80 | data.length); // masked + length
            } else if (data.length < 65536) {
                header = new byte[8];
                header[0] = (byte) 0x81;
                header[1] = (byte) 0xFE; // masked + 16-bit length
                header[2] = (byte) ((data.length >> 8) & 0xFF);
                header[3] = (byte) (data.length & 0xFF);
            } else {
                header = new byte[14];
                header[0] = (byte) 0x81;
                header[1] = (byte) 0xFF; // masked + 64-bit length
                for (int i = 0; i < 8; i++) {
                    header[8 - i] = (byte) ((data.length >> (i * 8)) & 0xFF);
                }
            }
            // 4-byte mask
            byte[] mask = new byte[4];
            SecureRandom random = new SecureRandom();
            random.nextBytes(mask);
            for (int i = 0; i < 4; i++) {
                header[header.length - 4 + i] = mask[i];
            }
            // Masked payload
            byte[] masked = new byte[data.length];
            for (int i = 0; i < data.length; i++) {
                masked[i] = (byte) (data[i] ^ mask[i % 4]);
            }
            synchronized (this) {
                if (mOutputStream != null) {
                    mOutputStream.write(header);
                    mOutputStream.write(masked);
                    mOutputStream.flush();
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "send failed: " + e.getMessage());
            close();
        }
    }

    /** Closes the connection. */
    public void close() {
        mRunning = false;
        try {
            // Send close frame
            byte[] closeFrame = new byte[2];
            closeFrame[0] = (byte) 0x88; // FIN + close
            closeFrame[1] = (byte) 0x80; // masked, 0 payload
            byte[] mask = new byte[4];
            SecureRandom random = new SecureRandom();
            random.nextBytes(mask);
            for (int i = 0; i < 4; i++) closeFrame[1 + i] = mask[i];
            synchronized (this) {
                if (mOutputStream != null) {
                    mOutputStream.write(closeFrame);
                    mOutputStream.flush();
                    mOutputStream.close();
                }
                if (mInputStream != null) mInputStream.close();
            }
        } catch (Exception e) {
            // ignore
        }
        mOutputStream = null;
        mInputStream = null;
    }

    public boolean isRunning() { return mRunning; }

    // --- internals ---

    private void readLoop() {
        try {
            byte[] buffer = new byte[8192];
            while (mRunning && mInputStream != null) {
                int count = mInputStream.read(buffer);
                if (count <= 0) {
                    break;
                }
                // Parse WebSocket frames
                int offset = 0;
                while (offset < count) {
                    byte b0 = buffer[offset];
                    byte b1 = buffer[offset + 1];
                    boolean fin = (b0 & 0x80) != 0;
                    int opcode = b0 & 0x0F;
                    boolean masked = (b1 & 0x80) != 0;
                    int payloadLen = b1 & 0x7F;
                    int headerLen = 2;

                    if (payloadLen == 126) {
                        payloadLen = ((buffer[offset + 2] & 0xFF) << 8) | (buffer[offset + 3] & 0xFF);
                        headerLen = 4;
                    } else if (payloadLen == 127) {
                        payloadLen = 0;
                        for (int i = 0; i < 8; i++) {
                            payloadLen = (payloadLen << 8) | (buffer[offset + 2 + i] & 0xFF);
                        }
                        headerLen = 10;
                    }

                    byte[] maskKey = null;
                    if (masked) {
                        maskKey = new byte[4];
                        for (int i = 0; i < 4; i++) {
                            maskKey[i] = buffer[offset + headerLen + i];
                        }
                        headerLen += 4;
                    }

                    int dataStart = offset + headerLen;
                    if (dataStart + payloadLen > count) break;

                    // Handle opcode
                    if (opcode == 0x8) { // Close
                        mRunning = false;
                    } else if (opcode == 0x9) { // Ping
                        // Pong back
                        byte[] pong = new byte[2];
                        pong[0] = (byte) 0x8A;
                        pong[1] = (byte) 0x80;
                        byte[] pongMask = new byte[4];
                        SecureRandom random = new SecureRandom();
                        random.nextBytes(pongMask);
                        for (int i = 0; i < 4; i++) pong[1 + i] = pongMask[i];
                        synchronized (this) {
                            if (mOutputStream != null) mOutputStream.write(pong);
                        }
                    } else if (opcode == 0x1 || opcode == 0x2) { // Text or Binary
                        byte[] payload = new byte[payloadLen];
                        for (int i = 0; i < payloadLen; i++) {
                            payload[i] = buffer[dataStart + i];
                            if (masked) payload[i] ^= maskKey[i % 4];
                        }
                        if (opcode == 0x1) {
                            String text = new String(payload, "UTF-8");
                            if (mOnMessage != null) {
                                mOnMessage.onMessage(text);
                            }
                        }
                    }

                    offset = dataStart + payloadLen;
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "read loop ended: " + e.getMessage());
        }
        mRunning = false;
        if (mOnError != null) {
            mOnError.onError("Connection lost");
        }
    }

    private String generateKey() {
        byte[] key = new byte[16];
        SecureRandom random = new SecureRandom();
        random.nextBytes(key);
        return java.util.Base64.getEncoder().encodeToString(key);
    }
}