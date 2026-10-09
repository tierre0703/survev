// Builds Unity-loadable sprite atlases from the web client's virtual atlases.
//
// The web client packs client/public/img/**/*.svg into PIXI atlases via
// client/atlas-builder; every "*.img" sprite name resolves to a frame. Unity
// needs static textures + frame rectangles, so this script reads the web atlas
// cache and emits PNG sheets + a frame index under Assets/Sprites/atlas/.
//
// Frame keys are kept verbatim as the web "*.img" names, so the runtime
// SpriteAtlasRegistry can resolve "loot-shirt-01.img" without translation.
//
// Usage:
//   node tools/build-atlas.mjs [--res low|high] [--out <dir>] [--only main,shared]
//
// NOTE: the web client build must have run once to populate
// ../client/node_modules/.atlas-cache/atlases.
import * as fs from "node:fs";
import { createRequire } from "node:module";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const clientDir = path.resolve(root, "..", "client");
const cacheRoot = path.join(clientDir, "node_modules", ".atlas-cache", "atlases");

const require = createRequire(path.join(clientDir, "package.json"));

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const res = argValue("--res", "low");
const outRoot = path.resolve(argValue("--out", path.join(root, "Assets", "Sprites", "atlas")));
const only = argValue("--only", "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean);

if (res !== "low" && res !== "high") {
    throw new Error(`--res must be low or high (got ${res})`);
}
if (!fs.existsSync(cacheRoot)) {
    throw new Error(
        `Atlas cache not found: ${cacheRoot}\nRun the web client build once to populate it.`,
    );
}

// Select the newest cached build of each atlas (dirs are "<atlas>-<hash>").
function latestAtlasDirs() {
    const byAtlas = new Map();
    for (const entry of fs.readdirSync(cacheRoot, { withFileTypes: true })) {
        if (!entry.isDirectory()) continue;
        const dash = entry.name.indexOf("-");
        if (dash < 0) continue;
        const atlas = entry.name.slice(0, dash);
        if (atlas === "atlas") continue;
        const full = path.join(cacheRoot, entry.name);
        if (!fs.existsSync(path.join(full, "data.json"))) continue;
        const prev = byAtlas.get(atlas);
        if (!prev || fs.statSync(full).mtimeMs > fs.statSync(prev).mtimeMs) {
            byAtlas.set(atlas, full);
        }
    }
    return byAtlas;
}

async function main() {
    const sharp = require("sharp");
    fs.mkdirSync(outRoot, { recursive: true });

    const atlases = latestAtlasDirs();
    let sheets = 0;
    let frames = 0;
    const index = [];

    for (const [atlas, dir] of atlases) {
        if (only.length && !only.includes(atlas)) continue;
        const data = JSON.parse(fs.readFileSync(path.join(dir, "data.json"), "utf8"));
        const sheetsForRes = data[res] || [];
        for (const sheet of sheetsForRes) {
            const imageName = sheet.meta.image;
            const src = path.join(dir, imageName);
            if (!fs.existsSync(src)) {
                continue;
            }
            const base = imageName.replace(/\.webp$/i, "");
            const pngName = `${base}.png`;
            const pngPath = path.join(outRoot, pngName);
            await sharp(src).png().toFile(pngPath);

            // Emit a simple frame index Unity can parse: frame name -> rect.
            const frameData = {};
            for (const [name, frame] of Object.entries(sheet.frames)) {
                frameData[name] = {
                    x: frame.frame.x,
                    y: frame.frame.y,
                    w: frame.frame.w,
                    h: frame.frame.h,
                    rotated: !!frame.rotated,
                    sourceW: frame.sourceSize.w,
                    sourceH: frame.sourceSize.h,
                    offsetX: frame.spriteSourceSize.x,
                    offsetY: frame.spriteSourceSize.y,
                };
                frames++;
            }
            const jsonName = `${base}.frames.json`;
            fs.writeFileSync(
                path.join(outRoot, jsonName),
                JSON.stringify({
                    image: pngName,
                    scale: sheet.meta.scale ?? 1,
                    frames: frameData,
                }),
            );

            index.push(jsonName);
            sheets++;
        }
    }

    fs.writeFileSync(path.join(outRoot, "index.txt"), index.join("\n") + "\n");
    console.log(`done: ${sheets} sheets, ${frames} frames -> ${outRoot}`);
}

main().catch((err) => {
    console.error(err.message);
    process.exit(1);
});
