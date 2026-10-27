// Generates GeneratedPreviewAnims.java from the real animData.ts + melee defs.
//
// The Java main menu avatar (CharacterPreviewView) is a static composite of
// rasterized outfit/melee layers, so it cannot use the C++ Player skeleton.
// This script mirrors what the web preview (client/src/ui/opponentDisplay.ts)
// feeds the player renderer:
//   - IdlePoses rows (the melee def's `anim.idlePose` hand/foot bone origins),
//   - each melee's attack/deploy/idle anim name lists,
//   - the keyframe tracks of every animation those lists reference.
//
// Bones follow animData.ts Bones order: HandL, HandR, FootL, FootR, MeleeL,
// MeleeR. A Pose is {pivot, rot, pos}; a keyframe carries a bone mask so a
// frame only overrides the bones it names (the rest fall back to the idle pose).
//
// Usage:
//   node --experimental-transform-types tools/codegen_preview_anims.mjs
//
// Output: proj.android/app/src/dev/axmol/app/GeneratedPreviewAnims.java
import * as fs from "node:fs";
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
const { IdlePoses, Animations } = await import(pathToFile(clientDir, "src/animData.ts"));
const { math } = await import(pathToFile(sharedDir, "utils/math.ts"));

const BONE_COUNT = 6;

const esc = (s) => String(s ?? "").replace(/\\/g, "\\\\").replace(/"/g, "\\\"");
const str = (s) => `"${esc(s)}"`;
// Trim floats so the generated table stays small but keeps full precision for
// the values animData.ts actually uses (world units / radians).
const ff = (v) => {
    const x = Math.round(Number(v ?? 0) * 1e6) / 1e6;
    return Object.is(x, -0) ? "0" : String(x);
};

// PoseEasing ordinal in src/render/Defs.h (Linear is 0).
const easings = [
    [math.easeInSine, 1],
    [math.easeOutSine, 2],
    [math.easeInOutSine, 3],
    [math.easeOutQuart, 4],
    [math.easeOutBounce, 5],
    [math.easeOutQuad, 6],
];

// --- melees ---------------------------------------------------------------
let melees = 0;
const animNames = new Set();
let meleeRows = "";
let soundRows = "";
// opponentDisplay.ts looks the sound up on the active item: player.ts
// `animPlaySound` uses `itemDef.sound[<effect key>]`, and the outfit change
// plays `outfitDef.sound.pickup` on the ui channel.
const addSound = (type, key, sound) => {
    if (sound && sound !== "none") soundRows += `        {${str(type)},${str(key)},${str(sound)}},\n`;
};
for (const type of GameObjectDefs.getAllTypes()) {
    let def;
    try {
        def = GameObjectDefs.typeToDef(type);
    } catch {
        continue;
    }
    if (!def) continue;
    if (def.type === "outfit") {
        addSound(type, "pickup", def.sound?.pickup);
        continue;
    }
    if (def.type !== "melee") continue;
    const anim = def.anim || {};
    const attack = anim.attackAnims || [];
    const deploy = anim.deployAnims || [];
    const idle = anim.idleAnims || [];
    for (const name of [...attack, ...deploy, ...idle]) animNames.add(name);
    for (const key of ["swing", "deploy", "idle"]) addSound(type, key, def.sound?.[key]);
    // worldImg: player.ts renders the melee sprite anchor-centred with
    // `pivot = -(bone.pos + worldImg.pos)`; the preview needs rot/pos/scale so
    // it can place the (anchor-centred) rasterized layer like the C++ port.
    const img = def.worldImg || {};
    const pos = img.pos || { x: 0, y: 0 };
    const scale = img.scale || { x: 1, y: 1 };
    meleeRows += `        {${str(type)},${str(anim.idlePose || "fists")},${
        str(
            def.worldImg?.renderOnHand ? "1" : "0",
        )
    },${str(ff(img.rot))},${str(ff(pos.x))},${str(ff(pos.y))},${str(ff(scale.x))},${str(ff(scale.y))},${
        str(attack.join(","))
    },${str(deploy.join(","))},${str(idle.join(","))}},\n`;
    melees++;
}

// --- idle poses -----------------------------------------------------------
let poseRows = "";
// IdlePoses entries are Pose(pivot) only (animData.ts `new Pose(v2.create(...))`):
// the offset lives in `pivot`, exactly like the C++ port's bone offset maths.
for (const [name, bones] of Object.entries(IdlePoses)) {
    const cells = [];
    for (let i = 0; i < BONE_COUNT; i++) {
        const bone = bones[i];
        cells.push(
            `${ff(bone?.pivot?.x)},${ff(bone?.pivot?.y)},${ff(bone?.rot)},${ff(bone?.pos?.x)},${ff(bone?.pos?.y)}`,
        );
    }
    poseRows += `        {${str(name)},${str(cells.join("|"))}},\n`;
}

// --- animations referenced by the melees ----------------------------------
if (!animNames.has("fists")) animNames.add("fists"); // melees without anim lists
let animRows = "";
for (const name of [...animNames].sort()) {
    const animation = Animations[name];
    if (!animation) throw new Error(`animData.ts has no animation "${name}" (referenced by a melee def)`);
    const frames = animation.keyframes.map((frame) => {
        const mask = Object.keys(frame.bones).reduce((bits, key) => bits | (1 << Number(key)), 0);
        const easing = easings.find(([fn]) => fn === frame.easing)?.[1] ?? 0;
        if (frame.easing && easing === 0) throw new Error(`Unknown easing in ${name}`);
        const cells = [];
        for (let i = 0; i < BONE_COUNT; i++) {
            const bone = frame.bones[i];
            cells.push(
                `${ff(bone?.pivot?.x)},${ff(bone?.pivot?.y)},${ff(bone?.rot)},${ff(bone?.pos?.x)},${ff(bone?.pos?.y)}`,
            );
        }
        return `${ff(frame.time)},${easing},${mask},${cells.join(",")}`;
    });
    // Sound effects only: animMeleeCollision/animThrowableParticles have no
    // target in the menu avatar (there is no world to collide with).
    const effects = (animation.effects || [])
        .filter((effect) => effect.fn === "animPlaySound" && effect.args?.sound)
        .map((effect) => `${ff(effect.time)},${effect.args.sound}`);
    animRows += `        {${str(name)},${str(frames.join(";"))},${str(effects.join(";"))}},\n`;
}

const out = `// GENERATED by tools/codegen_preview_anims.mjs — do not hand-edit.
package dev.axmol.app;

/**
 * animData.ts data for the Java menu character preview (ui/opponentDisplay.ts):
 * idle poses, per-melee anim lists and the keyframes of every animation those
 * lists reference. Bones are animData.ts order: HandL, HandR, FootL, FootR,
 * MeleeL, MeleeR.
 */
final class GeneratedPreviewAnims {
    private GeneratedPreviewAnims() {}

    /** {idle pose name, "pivotX,pivotY,rot,posX,posY" x6 bones} */
    private static final String[][] POSES = {
${poseRows}    };

    /** {melee type, idlePose, renderOnHand, worldImg rot,posX,posY,scaleX,scaleY, attack, deploy, idle} */
    private static final String[][] MELEES = {
${meleeRows}    };

    /** {anim name, "time,easing,mask,pivotX,pivotY,rot,posX,posY x6" ; frame, "time,soundKey ; effect"} */
    private static final String[][] ANIMS = {
${animRows}    };

    /** {item type, sound key ("swing"/"deploy"/"idle"/"pickup"), soundDefs name} */
    private static final String[][] SOUNDS = {
${soundRows}    };

    static String pose(String name) {
        for (String[] row : POSES) {
            if (row[0].equals(name)) return row[1];
        }
        if (!"fists".equals(name)) return pose("fists");
        return "";
    }

    static String[] melee(String type) {
        for (String[] row : MELEES) {
            if (row[0].equals(type)) return row;
        }
        return null;
    }

    static String anim(String name) {
        for (String[] row : ANIMS) {
            if (row[0].equals(name)) return row[1];
        }
        return null;
    }

    /** Sound effects of the anim ("time,soundKey ; effect"), empty when none. */
    static String animEffects(String name) {
        for (String[] row : ANIMS) {
            if (row[0].equals(name)) return row[2];
        }
        return "";
    }

    /** soundDefs.ts name for an item's sound key, "" when the item has none. */
    static String sound(String type, String key) {
        for (String[] row : SOUNDS) {
            if (row[0].equals(type) && row[1].equals(key)) return row[2];
        }
        return "";
    }
}
`;

fs.writeFileSync(path.resolve(root, "proj.android/app/src/dev/axmol/app/GeneratedPreviewAnims.java"), out);
console.log(`wrote GeneratedPreviewAnims.java (${melees} melees, ${animNames.size} anims)`);
