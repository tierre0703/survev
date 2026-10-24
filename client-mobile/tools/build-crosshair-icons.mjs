// Rasterizes the web client's crosshair SVGs into per-stroke PNGs so the native
// client can draw the configured crosshair (axmol cannot render SVG).
//
// The web client uses the SVG as a CSS cursor: white fill replaced by the
// configured colour, stroke-width replaced by the configured stroke, and the
// 64x64 art scaled by `size`. Native draws a screen-space sprite with a tint
// and scale, so this script bakes the three stroke buckets the Java editor
// exposes (0/1/2) at 4x resolution.
//
// Usage:
//   node --experimental-transform-types tools/build-crosshair-icons.mjs [--out <dir>]
//
// Output: Content/gui/crosshair/<type>_s<stroke>.png
import * as fs from "node:fs";
import { createRequire } from "node:module";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const sharedDir = path.resolve(root, "..", "shared");
const clientDir = path.resolve(root, "..", "client");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}
const { CrosshairDefs } = await import(
    pathToFile(sharedDir, "defs/gameObjects/crosshairDefs.ts")
);

const require = createRequire(path.join(clientDir, "package.json"));
const sharp = require("sharp");

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "gui", "crosshair")));
const scale = 4; // 64px art -> 256px texture
const strokes = [0, 1, 2];

fs.mkdirSync(outRoot, { recursive: true });

let count = 0;
for (const [type, def] of Object.entries(CrosshairDefs)) {
    const code = String(def.code || "");
    if (!code) continue;
    for (const stroke of strokes) {
        const svg = code.replace(/stroke-width="\.5"/g, `stroke-width="${stroke}"`);
        const png = path.join(outRoot, `${type}_s${stroke}.png`);
        await sharp(Buffer.from(svg), { density: 72 * scale })
            .resize(64 * scale, 64 * scale, {
                fit: "contain",
                background: { r: 0, g: 0, b: 0, alpha: 0 },
            })
            .png({ compressionLevel: 9 })
            .toFile(png);
        count++;
    }
}

console.log(`wrote ${count} crosshair sprites (${Object.keys(CrosshairDefs).length} styles)`);
