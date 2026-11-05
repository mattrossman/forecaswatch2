import { alertMessage, type Event, HOUR, INTERVAL } from "./alert.ts";

const now = Date.parse("2026-11-04T12:00:00Z");

function event(
  id: number,
  time: number,
  error: string | null = null,
  provider: Event["provider"] = "openmeteo",
): Event {
  return {
    id,
    received_at: new Date(time).toISOString(),
    account_token_hash: `account-${id}`,
    provider,
    success: error === null,
    error,
  };
}

function assertEquals(actual: unknown, expected: unknown) {
  if (actual !== expected) {
    throw new Error(`Expected ${expected}, got ${actual}`);
  }
}

Deno.test("alerts when provider failures cross threshold", () => {
  const events = Array.from(
    { length: 10 },
    (_, i) => event(i + 1, now - 1, i < 2 ? "provider_data: timeout" : null),
  );
  assertEquals(
    alertMessage(events, now),
    "Provider: openmeteo: 2/10 failed (20.0%)",
  );
  assertEquals(alertMessage(events.slice(0, 9), now), null);
});

Deno.test("does not repeat an already elevated window", () => {
  const events = Array.from(
    { length: 10 },
    (_, i) =>
      event(i + 1, now - INTERVAL - 1, i < 2 ? "provider_data: timeout" : null),
  );
  assertEquals(alertMessage(events, now), null);
});

Deno.test("groups location failures across providers", () => {
  const events = Array.from(
    { length: 10 },
    (_, i) =>
      event(
        i + 1,
        now - 1,
        i < 2 ? "forward_geocode: timeout" : null,
        i % 2 ? "openmeteo" : "wunderground",
      ),
  );
  assertEquals(alertMessage(events, now), "Location: 2/10 failed (20.0%)");
});

Deno.test("uses the latest event in each window", () => {
  const events = Array.from(
    { length: 10 },
    (_, i) => event(i + 1, now - INTERVAL - 1, null),
  );
  events.push({
    ...event(11, now - 1, "coordinates: gps_1"),
    account_token_hash: "account-1",
  });
  events.push({
    ...event(12, now - 1, "coordinates: gps_1"),
    account_token_hash: "account-2",
  });
  events.push(event(13, now - HOUR - INTERVAL - 1, "provider_data: timeout"));
  assertEquals(alertMessage(events, now), "Location: 2/10 failed (20.0%)");
});
