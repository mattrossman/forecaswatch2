import type { Tables } from "../_shared/database.types.ts";

export type Event = Pick<
  Tables<"telemetry_weather_fetch">,
  "id" | "received_at" | "account_token_hash" | "provider" | "success" | "error"
>;

type Count = { total: number; failed: number };
const HOUR = 60 * 60 * 1000;
const INTERVAL = 15 * 60 * 1000;
const providerNames: Record<Event["provider"], string> = {
  wunderground: "Weather Underground",
  openweathermap: "OpenWeatherMap",
  openmeteo: "Open-Meteo",
  mock: "Mock",
};

function latestEventsByAccount(
  events: Event[],
  end: number,
): Map<string, Event> {
  const latest = new Map<string, Event>();
  for (const event of events) {
    const time = Date.parse(event.received_at);
    if (time < end - HOUR || time >= end) continue;
    const previous = latest.get(event.account_token_hash);
    const previousTime = previous ? Date.parse(previous.received_at) : 0;
    if (
      !previous || time > previousTime ||
      (time === previousTime && event.id > previous.id)
    ) {
      latest.set(event.account_token_hash, event);
    }
  }
  return latest;
}

function counts(events: Event[], end: number): Map<string, Count> {
  const result = new Map<string, Count>();
  for (const event of latestEventsByAccount(events, end).values()) {
    const locationFailure = !event.success &&
      /^(coordinates|forward_geocode|reverse_geocode):/.test(event.error ?? "");
    const providerKey = providerNames[event.provider];
    const providerCount = result.get(providerKey) ?? { total: 0, failed: 0 };
    providerCount.total++;
    if (!event.success && !locationFailure) providerCount.failed++;
    result.set(providerKey, providerCount);

    const locationCount = result.get("Location") ?? { total: 0, failed: 0 };
    locationCount.total++;
    if (locationFailure) locationCount.failed++;
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
  const time = new Date(now);
  const dailyReminder = time.getUTCHours() === 12 && time.getUTCMinutes() < 15;
  const lines: string[] = [];
  for (const [name, count] of current) {
    if (!elevated(count) || (!dailyReminder && elevated(previous.get(name)))) {
      continue;
    }
    const percentage = (count.failed * 100 / count.total).toFixed(1);
    if (name === "Location") {
      lines.push(
        `In the last hour, ${count.failed} of ${count.total} accounts (${percentage}%) had their latest weather update fail due to a location error.`,
      );
    } else {
      lines.push(
        `In the last hour, ${count.failed} of ${count.total} accounts using ${name} (${percentage}%) had their latest weather update fail.`,
      );
    }
  }
  return lines.length ? lines.join("\n") : null;
}

export { HOUR, INTERVAL };
