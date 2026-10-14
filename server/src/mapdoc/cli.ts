import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { type MapDefKey, MapDefs } from "../../../shared/defs/mapDefs.ts";
import { Config } from "../config.ts";
import { type CaptureMapOptions, generateMapDoc } from "./captureMap.ts";

const USAGE = `Capture a generated map as a map editor document (JSON).

Usage:
  tsx server/src/mapdoc/cli.ts [mapName] [--seed <uint32>] [--team-mode <solo|duo|squad>] [--out <file>|--stdout]

Options:
  mapName        MapDef key (default: main). One of: ${Object.keys(MapDefs).join(", ")}
  --seed         uint32 seed to generate. Omit to pick a random seed that has a river.
  --team-mode    solo (small map) or duo/squad (large map). Default: solo.
  --out          Output path. Default: maps/<mapName>-<seed>.json
  --stdout       Print the document instead of writing a file.
`;

function fail(message: string): never {
    console.error(message);
    console.error(USAGE);
    process.exit(1);
}

const argv = process.argv.slice(2);

if (argv.includes("--help") || argv.includes("-h")) {
    console.log(USAGE);
    process.exit(0);
}

let mapName: string | undefined;
let seed: number | undefined;
let teamMode: CaptureMapOptions["teamMode"] = "solo";
let out: string | undefined;
let toStdout = false;

for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    switch (arg) {
        case "--seed": {
            const value = Number.parseInt(argv[++i] ?? "", 10);
            if (!Number.isFinite(value) || value < 0) fail("--seed expects a uint32");
            seed = value >>> 0;
            break;
        }
        case "--team-mode": {
            const value = (argv[++i] ?? "").toLowerCase();
            if (value !== "solo" && value !== "duo" && value !== "squad") {
                fail(`Unknown --team-mode "${value}"`);
            }
            teamMode = value;
            break;
        }
        case "--out": {
            out = argv[++i];
            if (!out) fail("--out expects a path");
            break;
        }
        case "--stdout": {
            toStdout = true;
            break;
        }
        default: {
            if (arg.startsWith("-")) fail(`Unknown option "${arg}"`);
            if (mapName) fail(`Unexpected argument "${arg}"`);
            mapName = arg;
        }
    }
}

const mapKey = (mapName ?? "main") as MapDefKey;
if (!(mapKey in MapDefs)) {
    fail(`Unknown map "${mapKey}"`);
}

// `maps/` at the repo root; this file lives in `server/src/mapdoc`.
const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
// `Config` resolves `survev-config.hjson` relative to the nearest `package.json`,
// so the command has to run from the repo root no matter where it is invoked.
process.chdir(repoRoot);

// Quiet the generator's map-gen timings (dev config logs them at `debug`) so
// the only output is this command's own report. Warnings still come through.
Config.logging.logDate = false;
Config.logging.infoLogs = false;
Config.logging.debugLogs = false;

const doc = generateMapDoc({ mapName: mapKey, seed, teamMode });
const text = JSON.stringify(doc, null, 4);

if (toStdout) {
    console.log(text);
    process.exit(0);
}

const target = out ?? path.join("maps", `${mapKey}-${doc.seed}.json`);
fs.mkdirSync(path.dirname(target), { recursive: true });
fs.writeFileSync(target, `${text}\n`);

const counts = new Map<string, number>();
for (const entry of doc.objects) {
    counts.set(entry.type, (counts.get(entry.type) ?? 0) + 1);
}

console.log(`Wrote ${target}`);
console.log(`  map:     ${doc.mapName}`);
console.log(`  seed:    ${doc.seed}`);
console.log(`  size:    ${doc.width}x${doc.height}`);
console.log(`  rivers:  ${doc.rivers.length}`);
console.log(`  places:  ${doc.places.length}`);
console.log(`  patches: ${doc.groundPatches.length}`);
console.log(`  objects: ${doc.objects.length} (${counts.size} types)`);
