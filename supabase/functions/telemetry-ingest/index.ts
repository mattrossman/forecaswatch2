import { createClient } from "@supabase/supabase-js";
import { z } from "zod";

const MAX_BODY_BYTES = 4096;
const MAX_EVENTS_PER_HOUR = 60;

const providerSchema = z.enum(["wunderground", "openweathermap", "mock"]);

const settingsSchema = z
  .object({
    temperatureUnits: z.string().optional(),
    dayNightShading: z.boolean().optional(),
    provider: providerSchema.optional(),
    axisTimeFormat: z.string().optional(),
    timeFont: z.string().optional(),
    timeLeadingZero: z.boolean().optional(),
    timeShowAmPm: z.boolean().optional(),
    weekStartDay: z.string().optional(),
    firstWeek: z.string().optional(),
    showQt: z.boolean().optional(),
    vibe: z.boolean().optional(),
    btIcons: z.string().optional(),
  })
  .strip();

const telemetryPayloadSchema = z.object({
  eventType: z.literal("weather_fetch"),
  accountToken: z.string().trim().min(1, {
    message: "invalid_account_token",
  }),
  watchToken: z.string().nullable().optional(),
  provider: providerSchema,
  success: z.boolean(),
  errorStage: z.string().nullable(),
  errorCode: z.string().nullable(),
  countryCode: z.string().nullable(),
  settings: settingsSchema.default({}),
  appVersion: z.string().trim().min(1, { message: "invalid_app_version" }),
  buildProfile: z.string().trim().min(1, {
    message: "invalid_build_profile",
  }),
  watchPlatform: z.string().nullable(),
  watchModel: z.string().nullable(),
});

type TelemetryPayload = z.infer<typeof telemetryPayloadSchema>;

function encodeUtf8(value: string) {
  return new TextEncoder().encode(value);
}

async function hmacSha256Hex(secret: string, message: string) {
  const key = await crypto.subtle.importKey(
    "raw",
    encodeUtf8(secret),
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"],
  );

  const signature = await crypto.subtle.sign("HMAC", key, encodeUtf8(message));
  const bytes = new Uint8Array(signature);
  let out = "";
  for (const byte of bytes) {
    out += byte.toString(16).padStart(2, "0");
  }
  return out;
}

Deno.serve(async (req) => {
  if (req.method !== "POST") {
    return Response.json({ error: "method_not_allowed" }, { status: 405 });
  }

  const rawBody = await req.text();
  if (encodeUtf8(rawBody).length > MAX_BODY_BYTES) {
    return Response.json({ error: "payload_too_large" }, { status: 413 });
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(rawBody);
  } catch (_error) {
    return Response.json({ error: "invalid_json" }, { status: 400 });
  }

  const payloadResult = telemetryPayloadSchema.safeParse(parsed);
  if (!payloadResult.success) {
    return Response.json({
      error: "invalid_payload",
      detail: payloadResult.error.issues[0]?.message || "invalid_payload",
    }, { status: 400 });
  }

  const payload: TelemetryPayload = payloadResult.data;

  const supabaseUrl = Deno.env.get("SUPABASE_URL");
  const serviceRoleKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY");
  const telemetryHashSecret = Deno.env.get("TELEMETRY_HASH_SECRET");

  if (!supabaseUrl) {
    throw new Error("SUPABASE_URL is not set");
  }

  if (!serviceRoleKey) {
    throw new Error("SUPABASE_SERVICE_ROLE_KEY is not set");
  }

  if (!telemetryHashSecret) {
    throw new Error("TELEMETRY_HASH_SECRET is not set");
  }

  const supabase = createClient(
    supabaseUrl,
    serviceRoleKey,
  );
  const accountTokenHash = await hmacSha256Hex(
    telemetryHashSecret,
    payload.accountToken,
  );
  const watchTokenHash = payload.watchToken && payload.watchToken.trim() !== ""
    ? await hmacSha256Hex(telemetryHashSecret, payload.watchToken)
    : null;
  const oneHourAgo = new Date(Date.now() - 60 * 60 * 1000).toISOString();

  const rate = await supabase
    .from("telemetry_weather_fetch")
    .select("id", { count: "exact", head: true })
    .eq("account_token_hash", accountTokenHash)
    .gte("received_at", oneHourAgo);

  if (rate.error) {
    return Response.json({ error: "rate_check_failed" }, { status: 500 });
  }

  if ((rate.count || 0) >= MAX_EVENTS_PER_HOUR) {
    return Response.json({ error: "rate_limit_exceeded" }, { status: 429 });
  }

  const insertResult = await supabase.from("telemetry_weather_fetch").insert({
    account_token_hash: accountTokenHash,
    watch_token_hash: watchTokenHash,
    provider: payload.provider,
    success: payload.success,
    error_stage: payload.errorStage,
    error_code: payload.errorCode,
    country_code: payload.countryCode,
    settings_json: payload.settings,
    app_version: payload.appVersion,
    build_profile: payload.buildProfile,
    watch_platform: payload.watchPlatform,
    watch_model: payload.watchModel,
  });

  if (insertResult.error) {
    return Response.json({ error: "insert_failed" }, { status: 500 });
  }

  return Response.json({ status: "accepted" }, { status: 202 });
});
