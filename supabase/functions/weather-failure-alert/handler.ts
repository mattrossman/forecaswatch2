import { withSupabase } from "@supabase/server";
import type { Database } from "../_shared/database.types.ts";
import { alertMessage, type Event, HOUR, INTERVAL } from "./alert.ts";

export const handleRequest = withSupabase<Database>(
  { auth: "secret" },
  async (request, ctx) => {
    if (request.method !== "POST") {
      return new Response("Method not allowed", { status: 405 });
    }

    const topic = Deno.env.get("NTFY_TOPIC");
    if (!topic) throw new Error("NTFY_TOPIC is not set");

    const db = ctx.supabaseAdmin;
    const now = Date.now();
    const events: Event[] = [];
    let lastId = 0;
    while (true) {
      const { data, error } = await db.from("telemetry_weather_fetch")
        .select("id,received_at,account_token_hash,provider,success,error")
        .gte("received_at", new Date(now - HOUR - INTERVAL).toISOString())
        .lt("received_at", new Date(now).toISOString())
        .gt("id", lastId).order("id").limit(1000);
      if (error) throw new Error(`Telemetry query failed: ${error.message}`);
      events.push(...data);
      if (data.length < 1000) break;
      lastId = data[data.length - 1].id;
    }

    const message = alertMessage(events, now);
    if (message) {
      const response = await fetch(
        `https://ntfy.sh/${encodeURIComponent(topic)}`,
        {
          method: "POST",
          headers: { "Title": "Weather fetch failures" },
          body: message,
        },
      );
      if (!response.ok) {
        throw new Error(`ntfy request failed: ${response.status}`);
      }
    }
    return Response.json({ alerted: Boolean(message) });
  },
);
