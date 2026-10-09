// Rasterizes the web client's SVG GUI art (client/public/img/gui/*.svg) into
// high-resolution PNGs under Content/gui/, so the native axmol UI can draw the
// exact same icons/buttons the web client does (axmol cannot load SVG).
//
// The web client sizes these icons with CSS background-size (e.g. `.btn-settings`
// uses `background-size: 44px`); the script renders each SVG at `--scale` times
// its intrinsic size (default 4x) so the icon stays crisp on high-DPI phones and
// under the 0.5 content scale factor the app sets for the low-res game atlas.
//
// Usage:
//   node tools/build-gui-icons.mjs [--scale 4] [--out <dir>] [--only cog,news]
//
// Output: Content/gui/<name>.png  (one PNG per SVG)
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const clientDir = path.resolve(root, "..", "client");
const srcDir = path.join(clientDir, "public", "img", "gui");

const require = createRequire(path.join(clientDir, "package.json"));
const sharp = require("sharp");

function argValue(name, fallback) {
    const i = process.argv.indexOf(name);
    return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}
const scale = Math.max(1, Number(argValue("--scale", "4")) || 4);
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "gui")));
const only = argValue("--only", "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean);

if (!fs.existsSync(srcDir)) {
    throw new Error(`GUI source not found: ${srcDir}`);
}

// A handful of icons are referenced from `client/src/en.json`/CSS by a name that
// differs from the file name (the flag/emote art lives in another directory);
// the ones the native menu needs keep their file name.
const files = fs.readdirSync(srcDir)
    .filter((f) => f.toLowerCase().endsWith(".svg"))
    .filter((f) => only.length === 0 || only.includes(path.basename(f, ".svg")))
    .sort();

if (files.length === 0) {
    throw new Error("No SVG icons matched");
}

fs.mkdirSync(outRoot, { recursive: true });

let count = 0;
for (const file of files) {
    const name = path.basename(file, ".svg");
    const src = path.join(srcDir, file);
    const svg = fs.readFileSync(src);
    // Render at a fixed high resolution (the SVGs are 128x128 viewBox art) and
    // let axmol scale the sprite down; this keeps text/small icons sharp.
    const px = Math.round(128 * scale);
    const png = path.join(outRoot, `${name}.png`);
    try {
        await sharp(svg, { density: 72 * scale })
            .resize(px, px, { fit: "contain", background: { r: 0, g: 0, b: 0, alpha: 0 } })
            .png({ compressionLevel: 9 })
            .toFile(png);
        count++;
    } catch (error) {
        console.warn(`  ! ${name}: ${error.message}`);
    }
}
console.log(`wrote ${count} GUI icons (${scale}x, ${128 * scale}px) to ${outRoot}`);
