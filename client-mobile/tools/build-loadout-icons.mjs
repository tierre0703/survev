// Rasterizes the loadout item art the web client shows in the customize screen
// (client/src/helpers.ts getSvgFromGameType) into PNGs for the Android Java UI
// (Java cannot render SVG).
//
// Web source -> native output:
//   outfit/melee  public/img/loot/<lootImg.sprite>.svg       -> Content/img/loadout/<type>.png
//   heal/boost    public/img/particles/<texture>.svg         -> Content/img/loadout/<type>.png
//   emote         public/img/emotes/<texture>.svg            -> Content/img/loadout/<type>.png
//   crosshair     public/img/crosshairs/<texture>.svg        -> Content/img/loadout/<type>.png
//
// Melee loot art is rotated/mirrored here because the web applies the same CSS
// transform to the tile (`getCssTransformFromGameType`). Outfit tint is applied
// at draw time by Java (the shared LoadoutItemDef carries lootImg.tint), which
// matches the web's SVG multiply filter.
//
// Usage:
//   node --experimental-transform-types tools/build-loadout-icons.mjs [--out <dir>]
//
// Output: Content/img/loadout/<type>.png (128x128, transparent)
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

const { GameObjectDefs } = await import(pathToFile(sharedDir, "defs/register.ts"));
const require = createRequire(path.join(clientDir, "package.json"));
const sharp = require("sharp");

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "img", "loadout")));
const size = 128;
const categories = new Set(["outfit", "melee", "emote", "heal_effect", "boost_effect", "crosshair"]);
const publicDir = path.join(clientDir, "public");

function svgSource(def) {
    if (!def) return "";
    switch (def.type) {
        case "outfit":
        case "melee":
            return def.lootImg?.sprite ? `img/loot/${def.lootImg.sprite.slice(0, -4)}.svg` : "";
        case "heal_effect":
        case "boost_effect":
            return def.texture ? `img/particles/${def.texture.slice(0, -4)}.svg` : "";
        case "emote":
            return def.texture ? `img/emotes/${def.texture.slice(0, -4)}.svg` : "";
        case "crosshair":
            return def.texture ? `img/crosshairs/${def.texture.slice(0, -4)}.svg` : "";
        default:
            return "";
    }
}

fs.mkdirSync(outRoot, { recursive: true });

let written = 0;
const missing = [];
for (const type of GameObjectDefs.getAllTypes()) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        def = null;
    }
    if (!def || !categories.has(def.type)) continue;
    const rel = svgSource(def);
    if (!rel) {
        missing.push(`${type} (no source)`);
        continue;
    }
    const src = path.join(publicDir, rel);
    if (!fs.existsSync(src)) {
        missing.push(`${type} (${rel})`);
        continue;
    }
    let pipeline = sharp(src, { density: 72 * 4 });
    const rot = Number(def.lootImg?.rot || 0);
    const mirror = !!def.lootImg?.mirror;
    // CSS `rotate(θ) scaleX(-1)` applies the mirror first.
    if (mirror) pipeline = pipeline.flop();
    if (rot) {
        pipeline = pipeline.rotate((rot * 180) / Math.PI, {
            background: { r: 0, g: 0, b: 0, alpha: 0 },
        });
    }
    await pipeline
        .resize(size, size, {
            fit: "contain",
            background: { r: 0, g: 0, b: 0, alpha: 0 },
        })
        .png({ compressionLevel: 9 })
        .toFile(path.join(outRoot, `${type}.png`));
    written++;
}

if (missing.length) {
    console.warn(`loadout icons: ${missing.length} item(s) had no SVG:`);
    for (const item of missing.slice(0, 20)) console.warn(`  - ${item}`);
    if (missing.length > 20) console.warn(`  ... and ${missing.length - 20} more`);
}
console.log(`wrote ${written} loadout icons to ${outRoot}`);
