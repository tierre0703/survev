import { type Item, type Loadout } from "../utils/loadout.ts";

export type ProfileResponse =
    | {
        readonly banned: true;
        reason: string;
        success?: false;
    }
    | {
        banned?: false;
        readonly success: true;
        profile: {
            slug: string;
            username: string;
            usernameSet: boolean;
            linked: boolean;
            usernameChangeTime: number;
        };
        loadout: Loadout;
        items: Item[];
    };

export type UsernameResponse =
    | {
        result: "success";
    }
    | {
        result: "failed" | "invalid" | "taken" | "change_time_not_expired";
    };

/**
 * Native device login (no OAuth): the client posts its persisted install id and
 * receives a session token for `Authorization: Bearer <token>`.
 */
export type DeviceLoginResponse =
    | {
        success: true;
        token: string;
    }
    | {
        success: false;
        error: "invalid_device";
    };

//
// PASS
//

export type PassState = {
    type: string;
    level: number;
    xp: number;
    unlocks: Record<string, boolean>;
    newItems: boolean;
};

export type QuestState = {
    idx: number;
    type: string;
    progress: number;
    target: number;
    complete: boolean;
    rerolled: boolean;
    timeToRefresh: number;
};

export type GetPassResponse = {
    success: true;
    pass: PassState;
    quests: QuestState[];
};
