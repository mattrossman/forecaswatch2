import type { Tables } from "../_shared/database.types.ts";

export type Event = Pick<
  Tables<"telemetry_weather_fetch">,
  "id" | "received_at" | "account_token_hash" | "provider" | "success" | "error"
>;

type Count = { total: number; failed: number };
const HOUR = 60 * 60 * 1000;
const INTERVAL = 15 * 60 * 1000;

function counts(events: Event[], end: number): Map<string, Count> {
  const latest = new Map<string, Event>();
  for (const event of events) {
    const time = Date.parse(event.received_at);
    if (time < end - HOUR || time >= end) continue;
    const previous = latest.get(event.account_token_hash);
    if (
      !previous || time > Date.parse(previous.received_at) ||
      (time === Date.parse(previous.received_at) && event.id > previous.id)
    ) {
      latest.set(event.account_token_hash, event);
    }
  }

  const result = new Map<string, Count>();
  for (const event of latest.values()) {
    const location = !event.success &&
      /^(coordinates|forward_geocode|reverse_geocode):/.test(event.error ?? "");
    const providerKey = `Provider: ${event.provider}`;
    const providerCount = result.get(providerKey) ?? { total: 0, failed: 0 };
    providerCount.total++;
    if (!event.success && !location) providerCount.failed++;
    result.set(providerKey, providerCount);

    const locationCount = result.get("Location") ?? { total: 0, failed: 0 };
    locationCount.total++;
    if (location) locationCount.failed++;
    result.set("Location", locationCount);
  }
  return result;
}

function elevated(count: Count | undefined): boolean {
  return Boolean(
    count && count.total >= 10 && count.failed * 100 >= count.total * 15,
  );
}

export function alertMessage(events: Event[], now: number): string | null {
  const current = counts(events, now);
  const previous = counts(events, now - INTERVAL);
  const lines: string[] = [];
  for (const [name, count] of current) {
    if (elevated(count) && !elevated(previous.get(name))) {
      lines.push(
        `${name}: ${count.failed}/${count.total} failed (${
          (count.failed * 100 / count.total).toFixed(1)
        }%)`,
      );
    }
  }
  return lines.length ? lines.join("\n") : null;
}

export { HOUR, INTERVAL };
