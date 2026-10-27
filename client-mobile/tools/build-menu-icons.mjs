// Rasterizes the web client's quick-play mode icons
// (shared/defs/maps/*.ts MapDef.desc.icon) into PNGs for the Java menu
// (Java cannot render SVG).
//
// Web source -> native output:
//   client/public/<MapDef.desc.icon> -> Content/img/menu/<mapName>.png
//
// Usage:
//   node --experimental-transform-types tools/build-menu-icons.mjs [--out <dir>]
//
// Output: Content/img/menu/<mapName>.png (96x96, transparent)
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

const { MapDefs } = await import(pathToFile(sharedDir, "defs/mapDefs.ts"));
const require = createRequire(path.join(clientDir, "package.json"));
const sharp = require("sharp");

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "img", "menu")));
const publicDir = path.join(clientDir, "public");
const size = 96;

fs.mkdirSync(outRoot, { recursive: true });

let written = 0;
const missing = [];
for (const [name, def] of Object.entries(MapDefs)) {
    const icon = def?.desc?.icon || "";
    if (!icon) continue;
    const src = path.join(publicDir, icon);
    if (!fs.existsSync(src)) {
        missing.push(`${name} (${icon})`);
        continue;
    }
    await sharp(src, { density: 72 * 4 })
        .resize(size, size, {
            fit: "contain",
            background: { r: 0, g: 0, b: 0, alpha: 0 },
        })
        .png({ compressionLevel: 9 })
        .toFile(path.join(outRoot, `${name}.png`));
    written++;
}

if (missing.length) {
    console.warn(`menu icons: ${missing.length} map(s) had no source SVG:`);
    for (const item of missing) console.warn(`  - ${item}`);
}
console.log(`wrote ${written} menu icons to ${outRoot}`);
