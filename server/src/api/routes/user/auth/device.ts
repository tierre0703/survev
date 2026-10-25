import { eq } from "drizzle-orm";
import { Hono } from "hono";
import z from "zod";
import type { DeviceLoginResponse } from "../../../../../../shared/types/user.ts";
import { validateParams } from "../../../auth/middleware.ts";
import { db } from "../../../db/index.ts";
import { usersTable } from "../../../db/schema.ts";
import { createNewUser, generateAvailableUsername, generateId, setSessionTokenCookie } from "./authUtils.ts";

/**
 * Device login for the native client (M8).
 *
 * The Android app has no OAuth browser flow: on first launch it generates a
 * random install id, keeps it in its config, and posts it here. The id is the
 * account's stable credential, so a reinstall (which keeps the id in
 * SharedPreferences-backed config) resolves to the same user.
 *
 * The session token is returned in the JSON body because the native HTTP client
 * has no cookie jar; `authMiddleware` accepts it as a Bearer token.
 */
export const DeviceRouter = new Hono();

export const DEVICE_AUTH_PREFIX = "device:";

DeviceRouter.post(
    "/",
    validateParams(
        z.object({
            deviceId: z
                .string()
                .trim()
                .min(6)
                // UUIDs, Android IDs and the fallback random ids all fit this.
                .max(128)
                .regex(/^[A-Za-z0-9_-]+$/),
        }),
        { success: false, error: "invalid_device" } satisfies DeviceLoginResponse,
    ),
    async (c) => {
        const { deviceId } = c.req.valid("json");
        const authId = DEVICE_AUTH_PREFIX + deviceId;

        const existingUser = await db.query.usersTable.findFirst({
            where: eq(usersTable.authId, authId),
            columns: {
                id: true,
            },
        });

        let userId = existingUser?.id;

        if (!userId) {
            const { username, slug } = await generateAvailableUsername();

            userId = generateId(15);
            // `linked` stays false: the account has no external provider. The
            // device credential is still enough to log in from the app.
            await createNewUser({
                id: userId,
                authId,
                username,
                slug,
                linked: false,
            });
        }

        const { token } = await setSessionTokenCookie(userId, c);

        return c.json<DeviceLoginResponse>({ success: true, token }, 200);
    },
);
