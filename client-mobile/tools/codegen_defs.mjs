import { fileURLToPath } from "node:url";
import * as path from "node:path";
import * as fs from "node:fs";

const here = path.dirname(fileURLToPath(import.meta.url));
const sharedDir = path.resolve(here, "../../shared");

function pathToFile(dir, rel) {
    return new URL(`file:///${path.join(dir, rel).replace(/\\/g, "/")}`);
}

const { GameObjectDefs, MapObjectDefs } = await import(pathToFile(sharedDir, "defs/register.ts"));
const { GameConfig } = await import(pathToFile(sharedDir, "gameConfig.ts"));

const gameTypes = GameObjectDefs.getAllTypes();
const mapTypes = MapObjectDefs.getAllTypes();
const bagSizeKeys = Object.keys(GameConfig.bagSizes);

let out = `// GENERATED FILE - do not edit. Run tools/codegen_defs.mjs to regenerate.
#pragma once
#include <array>
#include <string_view>

namespace surv {
namespace defs {

inline constexpr int kGameTypeBits = 10;
inline constexpr int kMapTypeBits = 12;
inline constexpr std::size_t kGameTypeCount = ${gameTypes.length + 1};
inline constexpr std::size_t kMapTypeCount = ${mapTypes.length + 1};

// Order is critical: "" = 0, then each type in Object.keys() merge order.
inline constexpr std::array<std::string_view, kGameTypeCount> kGameTypes = {
    "",
`;
for (const t of gameTypes) {
    out += `    "${t}",\n`;
}
out += `};

inline constexpr std::array<std::string_view, kMapTypeCount> kMapTypes = {
    "",
`;
for (const t of mapTypes) {
    out += `    "${t}",\n`;
}
out += `};

inline constexpr int kProtocolVersion = ${GameConfig.protocolVersion};
inline constexpr int kStructureLayerCount = ${GameConfig.structureLayerCount};
inline constexpr float kProjectileMaxHeight = ${GameConfig.projectile.maxHeight};

// Inventory item order must match Object.keys(GameConfig.bagSizes).
inline constexpr std::array<std::string_view, ${bagSizeKeys.length}> kBagSizeKeys = {
`;
for (const k of bagSizeKeys) {
    out += `    "${k}",\n`;
}
out += `};

} // namespace defs
} // namespace surv
`;

const dest = process.argv[2];
fs.mkdirSync(path.dirname(dest), { recursive: true });
fs.writeFileSync(dest, out);

console.log(`wrote ${dest}: ${gameTypes.length} game types, ${mapTypes.length} map types`);
console.log(`protocolVersion = ${GameConfig.protocolVersion}`);