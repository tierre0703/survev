// Rasterizes the menu character preview (client/src/ui/opponentDisplay.ts) into
// layered PNGs the Java main menu can draw without a PIXI/axmol context.
//
// The web preview is a live player render (idle pose + melee idle animations).
// Java gets the pieces of that skeleton it needs to animate:
//   body      outfit base + backpack (static, drawn in the body frame)
//   hand_l/r  the two hands, each centred on its own canvas so Java can place
//             them at the animated animData.ts HandL/HandR bone pose
//   melee     the selected melee art pivoted on the right hand (MeleeR bone)
//
// Web source -> native output:
//   client/public/img/player/*.svg  -> Content/img/preview/body_<type>.png
//   client/public/img/loot/*.svg    -> Content/img/preview/melee_<type>.png
//
// The canvas is 4x the web's world scale: 1 world unit = 4 px, the character
// centre is (256, 256), and the melee layer is centred on the melee sprite's
// PIXI pivot (animData.ts worldImg.pos).
//
// Usage:
//   node --experimental-transform-types tools/build-character-preview.mjs [--out <dir>]
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
const outRoot = path.resolve(argValue("--out", path.join(root, "Content", "img", "preview")));
const publicRoot = path.join(clientDir, "public");
const canvas = 512;
const center = canvas / 2;
const worldScale = 4; // px per world unit
const backpack02 = { offset: 11.5, scale: 0.23 };

fs.mkdirSync(outRoot, { recursive: true });
// The directory is tool-owned: drop stale layers from older naming schemes so
// an APK can never package a hand/body layer the view no longer asks for.
for (const file of fs.readdirSync(outRoot)) {
    if (file.endsWith(".png")) fs.rmSync(path.join(outRoot, file));
}

// `player-base-01.img` -> `img/player/player-base-01.svg` (some skins use the
// loot art instead). The `.img` suffix is the only shared asset convention.
function svgForFrame(frame) {
    if (!frame || frame === "none") return "";
    const base = frame.replace(/\.img$/, "");
    for (const dir of ["player", "loot", "particles"]) {
        const rel = `img/${dir}/${base}.svg`;
        if (fs.existsSync(path.join(publicRoot, rel))) return rel;
    }
    return "";
}

async function renderSvg(rel) {
    const file = path.join(publicRoot, rel);
    // 288 dpi = 4x the SVG's 72 dpi natural size, i.e. one world unit = 4 px.
    const { data, info } = await sharp(fs.readFileSync(file), { density: 288 })
        .png()
        .toBuffer({ resolveWithObject: true });
    return { buf: data, width: info.width, height: info.height };
}

async function tintPng(input, color) {
    if (color === undefined || color === null || color === 0xffffff) return input;
    // PIXI `sprite.tint` multiplies the RGB channels and keeps alpha. sharp's
    // tint() only recolours grayscale art, so do the multiply on raw pixels.
    const { data, info } = await sharp(input).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
    const tr = (color >> 16) & 0xff;
    const tg = (color >> 8) & 0xff;
    const tb = color & 0xff;
    for (let i = 0; i < data.length; i += 4) {
        data[i] = (data[i] * tr) / 255;
        data[i + 1] = (data[i + 1] * tg) / 255;
        data[i + 2] = (data[i + 2] * tb) / 255;
    }
    return sharp(data, { raw: { width: info.width, height: info.height, channels: 4 } }).png().toBuffer();
}

function dataUri(buf) {
    return `data:image/png;base64,${buf.toString("base64")}`;
}

async function composeSvg(body) {
    return sharp(
        Buffer.from(`<svg xmlns="http://www.w3.org/2000/svg" width="${canvas}" height="${canvas}">${body}</svg>`),
    )
        .png()
        .toBuffer();
}

// One `<image>` transform: scale first, then rotate, with `pivot` placed at
// `(x, y)` in canvas pixels (PIXI sprite pivot semantics).
function imageTag(inner, x, y, scale, rot, pivotX, pivotY) {
    const pivot = `translate(${x} ${y}) rotate(${(rot * 180) / Math.PI}) scale(${scale}) `
        + `translate(${-pivotX} ${-pivotY})`;
    return `<g transform="${pivot}">${inner}</g>`;
}

async function outfitLayer(type, def) {
    const skin = def.skinImg;
    if (!skin || !skin.baseSprite) return false;
    const parts = [];
    const bagRel = svgForFrame(skin.backpackSprite);
    if (bagRel) {
        const bag = await renderSvg(bagRel);
        const bagArt = await tintPng(bag.buf, skin.backpackTint);
        parts.push(
            imageTag(
                `<image x="0" y="0" width="${bag.width}" height="${bag.height}" href="${dataUri(bagArt)}"/>`,
                center - backpack02.offset * worldScale,
                center,
                backpack02.scale,
                0,
                bag.width / 2,
                bag.height / 2,
            ),
        );
    }
    const baseRel = svgForFrame(skin.baseSprite);
    if (!baseRel) return false;
    const base = await renderSvg(baseRel);
    const body = await tintPng(base.buf, skin.baseTint);
    parts.push(
        imageTag(
            `<image x="0" y="0" width="${base.width}" height="${base.height}" href="${dataUri(body)}"/>`,
            center,
            center,
            0.25,
            0,
            base.width / 2,
            base.height / 2,
        ),
    );
    fs.writeFileSync(path.join(outRoot, `body_${type}.png`), await composeSvg(parts.join("")));

    // One layer per hand, each centred on the canvas so Java can translate it
    // to the animated HandL/HandR bone offset (animData.ts idle pose + keyframes).
    const hands = typeof skin.handSprite === "string"
        ? { left: skin.handSprite, right: skin.handSprite }
        : (skin.handSprite || {});
    let handsOk = false;
    for (const [side, frame] of [["l", hands.left], ["r", hands.right]]) {
        const rel = svgForFrame(frame);
        if (!rel) continue;
        const art = await renderSvg(rel);
        const handArt = await tintPng(art.buf, skin.handTint);
        const layer = imageTag(
            `<image x="0" y="0" width="${art.width}" height="${art.height}" href="${dataUri(handArt)}"/>`,
            center,
            center,
            0.175,
            0,
            art.width / 2,
            art.height / 2,
        );
        fs.writeFileSync(path.join(outRoot, `hand_${side}_${type}.png`), await composeSvg(layer));
        handsOk = true;
    }
    return handsOk;
}

async function meleeLayer(type, def) {
    const img = def.worldImg;
    if (!img || !img.sprite) return false;
    const rel = svgForFrame(img.sprite);
    if (!rel) return false;
    const art = await renderSvg(rel);
    // player.ts renders the melee sprite anchor-centred (`createSprite()` sets
    // anchor 0.5,0.5) with `pivot = -(bone.pos + worldImg.pos)`; the C++ port
    // therefore places the sprite *centre* at
    //   R(rot) * worldImg.pos * worldImg.scale - MeleeR.pivot
    // relative to the hand. Bake the centre + rotation + scale here (pivot 0),
    // so Java only applies the MeleeR bone. The worldImg offset itself stays in
    // GeneratedPreviewAnims (it is rotated with the bone each frame).
    const scale = img.scale ? img.scale.x : 1;
    const inner = `<image x="0" y="0" width="${art.width}" height="${art.height}" href="${dataUri(art.buf)}"/>`;
    const body = imageTag(inner, center, center, scale, img.rot || 0, 0, 0);
    fs.writeFileSync(path.join(outRoot, `melee_${type}.png`), await composeSvg(body));
    return true;
}

let outfits = 0;
let melees = 0;
for (const type of GameObjectDefs.getAllTypes()) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        continue;
    }
    if (!def) continue;
    if (def.type === "outfit" && await outfitLayer(type, def)) outfits++;
    if (def.type === "melee" && await meleeLayer(type, def)) melees++;
}
console.log(`wrote ${outfits} outfit + ${melees} melee preview layers to ${outRoot}`);
