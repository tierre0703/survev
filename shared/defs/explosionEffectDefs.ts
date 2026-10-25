// Rendering-only explosion effect table, shared by the web client and the
// native codegen (client-mobile/tools/codegen_render_defs.mjs).
// Extracted from client/src/objects/explosion.ts so the native port can generate
// the same burst/scatter/ripple/shake/lifetime values.

export interface ExplotionDef {
    burst: {
        particle: string;
        scale: number;
        sound: {
            grass: string;
            water: string;
            detune?: number;
            volume?: number;
        };
    };
    scatter?: {
        particle: string;
        count: number;
        speed: {
            min: number;
            max: number;
        };
    };
    rippleCount: number;
    shakeStr: number;
    shakeDur: number;
    lifetime: number;
}

export const ExplosionEffectDefs: Record<string, ExplotionDef> = {
    frag: {
        burst: {
            particle: "explosionBurst",
            scale: 1,
            sound: {
                grass: "explosion_01",
                water: "explosion_02",
            },
        },
        rippleCount: 10,
        shakeStr: 0.2,
        shakeDur: 0.35,
        lifetime: 2,
    },
    smoke: {
        burst: {
            particle: "explosionBurst",
            scale: 0,
            sound: {
                grass: "explosion_smoke_01",
                water: "explosion_smoke_01",
            },
        },
        rippleCount: 10,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 6,
    },
    strobe: {
        burst: {
            particle: "explosionBurst",
            scale: 0.25,
            sound: {
                grass: "explosion_04",
                water: "explosion_02",
            },
        },
        rippleCount: 3,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 2,
    },
    barrel: {
        burst: {
            particle: "explosionBurst",
            scale: 1,
            sound: {
                grass: "explosion_01",
                water: "explosion_02",
            },
        },
        rippleCount: 10,
        shakeStr: 0.2,
        shakeDur: 0.35,
        lifetime: 2,
    },
    usas: {
        burst: {
            particle: "explosionUSAS",
            scale: 0.75,
            sound: {
                grass: "explosion_03",
                water: "explosion_02",
            },
        },
        rippleCount: 10,
        shakeStr: 0.12,
        shakeDur: 0.25,
        lifetime: 1.25,
    },
    rounds: {
        burst: {
            particle: "explosionRounds",
            scale: 0.32,
            sound: {
                grass: "explosion_04",
                water: "explosion_04",
                detune: 500,
                volume: 0.5,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    rounds_sg: {
        burst: {
            particle: "explosionRounds",
            scale: 0.32,
            sound: {
                grass: "explosion_04",
                water: "explosion_04",
                detune: 500,
                volume: 0.2,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    mirv: {
        burst: {
            particle: "explosionMIRV",
            scale: 1,
            sound: {
                grass: "explosion_01",
                water: "explosion_02",
            },
        },
        rippleCount: 10,
        shakeStr: 0.2,
        shakeDur: 0.35,
        lifetime: 2,
    },
    mirv_mini: {
        burst: {
            particle: "explosionMIRV",
            scale: 0.75,
            sound: {
                grass: "explosion_03",
                water: "explosion_02",
            },
        },
        rippleCount: 3,
        shakeStr: 0.1,
        shakeDur: 0.2,
        lifetime: 1.25,
    },
    martyr_nade: {
        burst: {
            particle: "explosionBurst",
            scale: 0.75,
            sound: {
                grass: "explosion_03",
                water: "explosion_02",
            },
        },
        rippleCount: 3,
        shakeStr: 0.1,
        shakeDur: 0.2,
        lifetime: 1.25,
    },
    snowball: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "snowball_01",
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "snowball_impact",
            count: 5,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    snowball_heavy: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "snowball_02",
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "snowball_impact",
            count: 8,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    potato: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "potato_01",
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "potato_impact",
            count: 5,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    potato_heavy: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "potato_02",
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "potato_impact",
            count: 8,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    potato_cannonball: {
        burst: {
            particle: "explosionPotato",
            scale: 0.75,
            sound: {
                grass: "explosion_05",
                water: "explosion_02",
            },
        },
        scatter: {
            particle: "potato_impact",
            count: 8,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 10,
        shakeStr: 0.12,
        shakeDur: 0.25,
        lifetime: 1.25,
    },
    potato_smgshot: {
        burst: {
            particle: "",
            scale: 0.2,
            sound: {
                grass: "potato_01",
                water: "potato_02",
                detune: 250,
                volume: 0.5,
            },
        },
        scatter: {
            particle: "potato_smg_impact",
            count: 2,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 0.5,
    },
    potato_lmgshot: {
        burst: {
            particle: "",
            scale: 0.1,
            sound: {
                grass: "potato_01",
                water: "potato_02",
                detune: 400,
                volume: 0.5,
            },
        },
        scatter: {
            particle: "potato_smg_impact",
            count: 1,
            speed: {
                min: 5,
                max: 20,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 0.3,
    },
    bomb_iron: {
        burst: {
            particle: "explosionBomb",
            scale: 2,
            sound: {
                grass: "explosion_01",
                water: "explosion_02",
            },
        },
        rippleCount: 12,
        shakeStr: 0.25,
        shakeDur: 0.4,
        lifetime: 2,
    },
    coconut: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "coconut_01",
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "coconut_impact",
            count: 6,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
    tomato: {
        burst: {
            particle: "",
            scale: 0.75,
            sound: {
                grass: "tomato_01", // TODO: add sound
                water: "frag_water_01",
            },
        },
        scatter: {
            particle: "tomato_impact",
            count: 4,
            speed: {
                min: 5,
                max: 25,
            },
        },
        rippleCount: 1,
        shakeStr: 0,
        shakeDur: 0,
        lifetime: 1,
    },
};
