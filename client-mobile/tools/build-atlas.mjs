// Converts the web client's built virtual atlases (client/node_modules/.atlas-cache)
// into axmol-loadable spritesheets: one PNG + cocos2d-format .plist per sheet.
//
// The atlas cache already contains the Pixi spritesheet JSON (`data.json`) and
// the packed .webp sheets produced by client/atlas-builder. This script does not
// rebuild the atlases; it re-encodes each sheet to PNG and writes a plist whose
// frame keys are the web client's `*.img` names, so AxSprite::setFrame()'s
// getSpriteFrameByName("loot-shirt-01.img") resolves directly.
//
// Output (staged into the APK's assets):
//   Content/atlas/<atlas>/<sheet>.png
//   Content/atlas/<atlas>/<sheet>.plist
//   Content/atlas/<atlas>/index.txt     (one plist path per line)
//
// Usage:
//   node tools/build-atlas.mjs [--res low|high] [--out <dir>] [--only main,shared]
//
// Resolution: `low` (0.5, the web client's mobile path) is the default. The
// plist geometry is emitted at the sheet's native pixel scale; the app sets
// Director::setContentScaleFactor(0.5) so low-res frames render at full size.
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const clientDir = path.resolve(root, "..", "client");
const cacheRoot = path.join(clientDir, "node_modules", ".atlas-cache", "atlases");

const require = createRequire(path.join(clientDir, "package.json"));
const sharp = require("sharp");

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const res = argValue("--res", "low");
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "atlas")));
const only = argValue("--only", "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean);

if (res !== "low" && res !== "high") {
    throw new Error(`--res must be low or high (got ${res})`);
}
if (!fs.existsSync(cacheRoot)) {
    throw new Error(`Atlas cache not found: ${cacheRoot}\nRun the web client build once to populate it.`);
}

const xmlEscape = (s) =>
    String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
const num = (x) => {
    const v = Number(x);
    return Number.isInteger(v) ? String(v) : String(Math.round(v * 1000) / 1000);
};

// Pick the newest cached build of each atlas (dirs are "<atlas>-<hash>").
function latestAtlasDirs() {
    const byAtlas = new Map();
    for (const entry of fs.readdirSync(cacheRoot, { withFileTypes: true })) {
        if (!entry.isDirectory()) continue;
        const dash = entry.name.indexOf("-");
        if (dash < 0) continue;
        const atlas = entry.name.slice(0, dash);
        const full = path.join(cacheRoot, entry.name);
        if (!fs.existsSync(path.join(full, "data.json"))) continue;
        const prev = byAtlas.get(atlas);
        if (!prev || fs.statSync(full).mtimeMs > fs.statSync(prev).mtimeMs) {
            byAtlas.set(atlas, full);
        }
    }
    return byAtlas;
}

// Pixi spriteSourceSize is the trimmed rect's top-left in the original image.
// cocos2d's `offset` is the trimmed content's centre relative to the original
// centre: offset = spriteSourceSize - (sourceSize - frameSize) / 2.
function frameOffset(sss, frame, source) {
    return {
        x: sss.x - (source.w - frame.w) / 2,
        y: sss.y - (source.h - frame.h) / 2,
    };
}

function writePlist(sheet, textureFile, destPlist) {
    const frames = sheet.frames || {};
    const size = sheet.meta.size || { w: 0, h: 0 };
    let out = `<?xml version="1.0" encoding="UTF-8"?>\n`;
    out += `<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n`;
    out += `<plist version="1.0">\n<dict>\n`;
    out += `\t<key>frames</key>\n\t<dict>\n`;
    for (const [name, f] of Object.entries(frames)) {
        const fr = f.frame;
        const sss = f.spriteSourceSize || { x: 0, y: 0 };
        const src = f.sourceSize || { w: fr.w, h: fr.h };
        const off = frameOffset(sss, fr, src);
        out += `\t\t<key>${xmlEscape(name)}</key>\n\t\t<dict>\n`;
        out += `\t\t\t<key>frame</key>\n\t\t\t<string>{{${num(fr.x)},${num(fr.y)}},{${num(fr.w)},${num(fr.h)}}}</string>\n`;
        out += `\t\t\t<key>offset</key>\n\t\t\t<string>{${num(off.x)},${num(off.y)}}</string>\n`;
        out += `\t\t\t<key>rotated</key>\n\t\t\t<${f.rotated ? "true" : "false"}/>\n`;
        out += `\t\t\t<key>sourceColorRect</key>\n\t\t\t<string>{{${num(sss.x)},${num(sss.y)}},{${num(fr.w)},${num(fr.h)}}}</string>\n`;
        out += `\t\t\t<key>sourceSize</key>\n\t\t\t<string>{${num(src.w)},${num(src.h)}}</string>\n`;
        out += `\t\t</dict>\n`;
    }
    out += `\t</dict>\n`;
    out += `\t<key>metadata</key>\n\t<dict>\n`;
    out += `\t\t<key>format</key>\n\t\t<integer>2</integer>\n`;
    out += `\t\t<key>size</key>\n\t\t<string>{${num(size.w)},${num(size.h)}}</string>\n`;
    out += `\t\t<key>textureFileName</key>\n\t\t<string>${xmlEscape(textureFile)}</string>\n`;
    out += `\t\t<key>realTextureFileName</key>\n\t\t<string>${xmlEscape(textureFile)}</string>\n`;
    out += `\t</dict>\n</dict>\n</plist>\n`;
    fs.writeFileSync(destPlist, out);
}

async function main() {
    const dirs = latestAtlasDirs();
    const atlasNames = [...dirs.keys()].filter((a) => only.length === 0 || only.includes(a)).sort();
    if (atlasNames.length === 0) {
        throw new Error("No atlases matched");
    }
    let sheetCount = 0;
    let frameCount = 0;
    for (const atlas of atlasNames) {
        const dir = dirs.get(atlas);
        const data = JSON.parse(fs.readFileSync(path.join(dir, "data.json"), "utf8"));
        const sheets = data[res] || [];
        const outDir = path.join(outRoot, atlas);
        fs.mkdirSync(outDir, { recursive: true });
        const indexLines = [];
        for (let i = 0; i < sheets.length; i++) {
            const sheet = sheets[i];
            const srcImage = path.join(dir, sheet.meta.image);
            if (!fs.existsSync(srcImage)) {
                console.warn(`  ! missing sheet ${srcImage}`);
                continue;
            }
            const base = sheet.meta.image.replace(/\.[^.]+$/, "");
            const png = `${base}.png`;
            const plist = `${base}.plist`;
            await sharp(srcImage).png({ compressionLevel: 9 }).toFile(path.join(outDir, png));
            writePlist(sheet, png, path.join(outDir, plist));
            indexLines.push(`atlas/${atlas}/${plist}`);
            sheetCount++;
            frameCount += Object.keys(sheet.frames || {}).length;
        }
        fs.writeFileSync(path.join(outDir, "index.txt"), indexLines.join("\n") + "\n");
        console.log(`  ${atlas}: ${sheets.length} sheet(s), ${indexLines.length} plist(s)`);
    }
    console.log(`wrote ${atlasNames.length} atlases (${sheetCount} sheets, ${frameCount} frames) to ${outRoot}`);
}

main().catch((e) => {
    console.error(e);
    process.exit(1);
});
