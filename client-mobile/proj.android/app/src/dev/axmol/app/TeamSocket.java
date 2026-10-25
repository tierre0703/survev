/****************************************************************************
 * TeamSocket.java - minimal WebSocket client for the /team_v2 JSON channel.
 *
 * The Gradle-free build vendors no WS library (ARCHITECTURE.md A6.2), and
 * java.net.HttpURLConnection cannot be trusted with a 101 protocol switch. This
 * class therefore uses a plain Socket/SSLSocket:
 *   connect (TCP/TLS) -> HTTP Upgrade handshake -> RFC 6455 frames.
 *
 * Only what the team menu needs is implemented: masked client text frames,
 * server text frames (with continuation reassembly), ping/pong and close.
 * Team messages are `{"type": "...", "data": {...}}` JSON (shared/types/team.ts).
 *
 * Threading: connect() returns immediately; the handshake + read loop run on a
 * background thread. onOpen/onMessage/onError are called from that thread, so
 * callers must post to the UI thread (TeamFragment does).
 ****************************************************************************/
package dev.axmol.app;

import android.util.Base64;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import javax.net.ssl.SSLSocketFactory;

public class TeamSocket {
    private static final String TAG = "TeamSocket";
    private static final String WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    private static final int CONNECT_TIMEOUT_MS = 10000;
    private static final int MAX_PAYLOAD = 1024 * 1024;

    public interface OnOpen {
        void onOpen();
    }

    public interface OnMessage {
        void onMessage(String text);
    }

    public interface OnError {
        void onError(String reason);
    }

    private Socket mSocket;
    private InputStream mIn;
    private OutputStream mOut;
    private Thread mThread;
    private Thread mWriter;
    /**
     * Callers include the UI thread, where network writes throw
     * NetworkOnMainThreadException. Frames are queued and written by mWriter.
     */
    private final LinkedBlockingQueue<String> mOutgoing = new LinkedBlockingQueue<String>();

    private volatile boolean mRunning = false;
    private volatile boolean mUserClosed = false;
    private boolean mErrorSent = false;

    private OnOpen mOnOpen;
    private OnMessage mOnMessage;
    private OnError mOnError;

    public TeamSocket() {}

    /**
     * Opens `ws://host:port/path` or `wss://...` asynchronously. `bearerToken`
     * may be null; the server accepts it in place of the web session cookie.
     * Returns false only when the URL itself is unusable.
     */
    public boolean connect(String url, String bearerToken,
                           OnOpen onOpen, OnMessage onMessage, OnError onError) {
        mOnOpen = onOpen;
        mOnMessage = onMessage;
        mOnError = onError;
        mUserClosed = false;
        mErrorSent = false;

        final URI uri;
        try {
            uri = new URI(url);
        } catch (Exception e) {
            Log.w(TAG, "bad url " + url + ": " + e.getMessage());
            return false;
        }
        if (!"ws".equalsIgnoreCase(uri.getScheme()) && !"wss".equalsIgnoreCase(uri.getScheme())
                || uri.getHost() == null) {
            Log.w(TAG, "unsupported url " + url);
            return false;
        }

        final String token = bearerToken;
        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                runConnection(uri, token);
            }
        }, "team-socket");
        mThread.start();
        return true;
    }

    public void send(String text) {
        if (!mRunning || text == null) {
            return;
        }
        mOutgoing.offer(text);
    }

    public boolean isRunning() {
        return mRunning;
    }

    /** Intentionally disconnects (no error callback afterwards). */
    public void close() {
        mUserClosed = true;
        mRunning = false;
        if (mWriter != null) {
            mWriter.interrupt();
        }
        closeQuietly();
    }

    // --- connection ---------------------------------------------------------

    private void runConnection(URI uri, String bearerToken) {
        try {
            boolean secure = "wss".equalsIgnoreCase(uri.getScheme());
            String host = uri.getHost();
            int port = uri.getPort();
            if (port < 0) {
                port = secure ? 443 : 80;
            }
            mSocket = secure
                    ? SSLSocketFactory.getDefault().createSocket()
                    : new Socket();
            mSocket.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT_MS);
            mSocket.setSoTimeout(0);
            if (mSocket instanceof javax.net.ssl.SSLSocket) {
                ((javax.net.ssl.SSLSocket) mSocket).startHandshake();
            }

            mIn = new BufferedInputStream(mSocket.getInputStream());
            mOut = new BufferedOutputStream(mSocket.getOutputStream());

            String path = uri.getRawPath();
            if (path == null || path.isEmpty()) {
                path = "/";
            }
            if (uri.getRawQuery() != null) {
                path += "?" + uri.getRawQuery();
            }

            String key = newKey();
            StringBuilder req = new StringBuilder();
            req.append("GET ").append(path).append(" HTTP/1.1\r\n");
            req.append("Host: ").append(host);
            if ((secure && port != 443) || (!secure && port != 80)) {
                req.append(':').append(port);
            }
            req.append("\r\n");
            req.append("Upgrade: websocket\r\n");
            req.append("Connection: Upgrade\r\n");
            req.append("Sec-WebSocket-Key: ").append(key).append("\r\n");
            req.append("Sec-WebSocket-Version: 13\r\n");
            req.append("Origin: https://survev.io\r\n");
            if (bearerToken != null && !bearerToken.isEmpty()) {
                req.append("Authorization: Bearer ").append(bearerToken).append("\r\n");
            }
            req.append("\r\n");
            mOut.write(req.toString().getBytes(StandardCharsets.US_ASCII));
            mOut.flush();

            Map<String, String> headers = readHandshakeResponse();
            String status = headers.get(":status");
            String accept = headers.get("sec-websocket-accept");
            String expected = Base64.encodeToString(
                    MessageDigest.getInstance("SHA-1")
                            .digest((key + WS_GUID).getBytes(StandardCharsets.US_ASCII)),
                    Base64.NO_WRAP);
            if (status == null || !status.contains("101")) {
                fail("upgrade rejected: " + status);
                closeQuietly();
                return;
            }
            if (accept == null || !accept.trim().equals(expected)) {
                fail("bad websocket accept");
                closeQuietly();
                return;
            }

            mRunning = true;
            startWriter();
            Log.i(TAG, "connected " + uri);
            if (mOnOpen != null) {
                mOnOpen.onOpen();
            }
            readLoop();
        } catch (Exception e) {
            if (!mUserClosed) {
                Log.w(TAG, "connection failed: " + e.getMessage());
                fail(e.getMessage() != null ? e.getMessage() : "connection failed");
            }
        } finally {
            mRunning = false;
            if (mWriter != null) {
                mWriter.interrupt();
            }
            closeQuietly();
            if (!mUserClosed) {
                fail("lost connection");
            }
        }
    }

    private void startWriter() {
        mWriter = new Thread(new Runnable() {
            @Override
            public void run() {
                writerLoop();
            }
        }, "team-socket-writer");
        mWriter.start();
    }

    private void writerLoop() {
        try {
            while (mRunning) {
                String msg = mOutgoing.take();
                writeFrame(0x1, msg.getBytes(StandardCharsets.UTF_8));
            }
        } catch (InterruptedException e) {
            // closing
        } catch (Exception e) {
            if (!mUserClosed) {
                Log.w(TAG, "send failed: " + e);
                fail("send failed");
            }
            mRunning = false;
            closeQuietly();
        }
    }

    private Map<String, String> readHandshakeResponse() throws Exception {
        ByteArrayOutputStream head = new ByteArrayOutputStream();
        int matched = 0;
        while (matched < 4) {
            int b = mIn.read();
            if (b < 0) {
                throw new Exception("connection closed during handshake");
            }
            head.write(b);
            if (head.size() > 16 * 1024) {
                throw new Exception("handshake too large");
            }
            if ((matched == 0 || matched == 2) && b == '\r') {
                matched++;
            } else if ((matched == 1 || matched == 3) && b == '\n') {
                matched++;
            } else {
                matched = 0;
            }
        }
        String text = head.toString("US-ASCII");
        String[] lines = text.split("\r\n");
        Map<String, String> headers = new HashMap<String, String>();
        if (lines.length > 0) {
            headers.put(":status", lines[0]);
        }
        for (int i = 1; i < lines.length; i++) {
            int colon = lines[i].indexOf(':');
            if (colon > 0) {
                headers.put(
                        lines[i].substring(0, colon).trim().toLowerCase(Locale.US),
                        lines[i].substring(colon + 1).trim());
            }
        }
        return headers;
    }

    // --- frames -------------------------------------------------------------

    private void readLoop() throws Exception {
        ByteArrayOutputStream fragments = null;
        int fragmentOpcode = 0;

        while (mRunning && !mUserClosed) {
            int b0 = mIn.read();
            int b1 = mIn.read();
            if (b0 < 0 || b1 < 0) {
                break;
            }
            boolean fin = (b0 & 0x80) != 0;
            int opcode = b0 & 0x0F;
            boolean masked = (b1 & 0x80) != 0;
            long len = b1 & 0x7F;
            if (len == 126) {
                len = ((long) readByte() << 8) | readByte();
            } else if (len == 127) {
                len = 0;
                for (int i = 0; i < 8; i++) {
                    len = (len << 8) | readByte();
                }
            }
            if (len < 0 || len > MAX_PAYLOAD) {
                throw new Exception("frame too large: " + len);
            }
            byte[] mask = null;
            if (masked) {
                mask = new byte[4];
                readFully(mask);
            }
            byte[] payload = new byte[(int) len];
            readFully(payload);
            if (mask != null) {
                for (int i = 0; i < payload.length; i++) {
                    payload[i] ^= mask[i % 4];
                }
            }

            switch (opcode) {
                case 0x0: // continuation
                    if (fragments != null) {
                        fragments.write(payload);
                    }
                    if (fin && fragments != null) {
                        deliver(fragmentOpcode, fragments.toByteArray());
                        fragments = null;
                    }
                    break;
                case 0x1: // text
                case 0x2: // binary (unused by /team_v2)
                    if (fin) {
                        deliver(opcode, payload);
                    } else {
                        fragments = new ByteArrayOutputStream();
                        fragments.write(payload);
                        fragmentOpcode = opcode;
                    }
                    break;
                case 0x8: // close
                    mRunning = false;
                    try {
                        writeFrame(0x8, new byte[0]);
                    } catch (Exception e) {
                        // ignore
                    }
                    return;
                case 0x9: // ping
                    writeFrame(0xA, payload);
                    break;
                case 0xA: // pong
                    break;
                default:
                    break;
            }
        }
    }

    private void deliver(int opcode, byte[] payload) {
        if (opcode != 0x1 || mOnMessage == null) {
            return;
        }
        mOnMessage.onMessage(new String(payload, StandardCharsets.UTF_8));
    }

    private void writeFrame(int opcode, byte[] data) throws Exception {
        OutputStream out = mOut;
        if (out == null) {
            return;
        }
        int len = data.length;
        byte[] header;
        if (len < 126) {
            header = new byte[6];
            header[1] = (byte) (0x80 | len);
        } else if (len <= 0xFFFF) {
            header = new byte[8];
            header[1] = (byte) (0x80 | 126);
            header[2] = (byte) ((len >> 8) & 0xFF);
            header[3] = (byte) (len & 0xFF);
        } else {
            header = new byte[14];
            header[1] = (byte) (0x80 | 127);
            long longLen = len;
            for (int i = 0; i < 8; i++) {
                header[2 + i] = (byte) ((longLen >> (56 - i * 8)) & 0xFF);
            }
        }
        header[0] = (byte) (0x80 | (opcode & 0x0F)); // FIN + opcode

        byte[] mask = new byte[4];
        new SecureRandom().nextBytes(mask);
        int maskAt = header.length - 4;
        System.arraycopy(mask, 0, header, maskAt, 4);
        byte[] masked = new byte[len];
        for (int i = 0; i < len; i++) {
            masked[i] = (byte) (data[i] ^ mask[i % 4]);
        }

        synchronized (out) {
            out.write(header);
            out.write(masked);
            out.flush();
        }
    }

    private int readByte() throws Exception {
        int b = mIn.read();
        if (b < 0) {
            throw new Exception("connection closed");
        }
        return b;
    }

    private void readFully(byte[] buffer) throws Exception {
        int offset = 0;
        while (offset < buffer.length) {
            int read = mIn.read(buffer, offset, buffer.length - offset);
            if (read < 0) {
                throw new Exception("connection closed");
            }
            offset += read;
        }
    }

    private void fail(String reason) {
        if (mErrorSent || mUserClosed) {
            return;
        }
        mErrorSent = true;
        if (mOnError != null) {
            mOnError.onError(reason);
        }
    }

    private void closeQuietly() {
        try {
            if (mIn != null) mIn.close();
        } catch (Exception e) {
            // ignore
        }
        try {
            if (mOut != null) mOut.close();
        } catch (Exception e) {
            // ignore
        }
        try {
            if (mSocket != null) mSocket.close();
        } catch (Exception e) {
            // ignore
        }
        mIn = null;
        mOut = null;
        mSocket = null;
    }

    private static String newKey() {
        byte[] key = new byte[16];
        new SecureRandom().nextBytes(key);
        return Base64.encodeToString(key, Base64.NO_WRAP);
    }
}
