// Temporary probe: join a local game and dump the first messages' framing.
const res = await fetch("http://127.0.0.1:8000/api/find_game_v2", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({
        region: "local",
        zones: ["local"],
        version: 1028,
        playerCount: 1,
        autoFill: true,
        gameModeIdx: 0,
    }),
});
const body = await res.json();
console.log("find_game", JSON.stringify(body));
const url = body.res.urls[0];
const token = body.res.joinToken;

const ws = new WebSocket(url);
ws.binaryType = "arraybuffer";
const hex = (b) => Buffer.from(b).toString("hex").slice(0, 80);

ws.onopen = () => {
    const t = Buffer.from(token, "utf8");
    const payload = Buffer.alloc(2 + t.length);
    payload.writeUInt16LE(t.length, 0);
    t.copy(payload, 2);
    const buf = Buffer.alloc(3 + payload.length);
    buf.writeUInt8(0, 0);
    buf.writeUInt16LE(payload.length, 1);
    payload.copy(buf, 3);
    ws.send(buf);
    console.log("sent join", hex(buf));
};

let n = 0;
ws.onmessage = (ev) => {
    const buf = Buffer.from(ev.data);
    console.log("msg", n, "len", buf.length, "head", hex(buf));
    n++;
    if (n > 8) process.exit(0);
};
ws.onerror = (e) => console.log("error", e.message);
ws.onclose = (e) => {
    console.log("close", e.code, e.reason);
    process.exit(0);
};
setTimeout(() => process.exit(0), 10000);

// keep the event loop alive on Node versions without a built-in ws close path
setInterval(() => {}, 1000);
