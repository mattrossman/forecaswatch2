import { hmacSha256Hex } from "../_shared/hash.ts";

export async function handleRequest(req: Request): Promise<Response> {
  if (req.method !== "POST") {
    return Response.json({ error: "method_not_allowed" }, { status: 405 });
  }

  const rawBody = await req.text();
  if (new TextEncoder().encode(rawBody).length > 4096) {
    return Response.json({ error: "payload_too_large" }, { status: 413 });
  }

  let payload: unknown;
  try {
    payload = JSON.parse(rawBody);
  } catch {
    return Response.json({ error: "invalid_json" }, { status: 400 });
  }

  if (
    typeof payload !== "object" || payload === null ||
    !("accountToken" in payload) ||
    typeof payload.accountToken !== "string" ||
    payload.accountToken.trim() === ""
  ) {
    return Response.json({ error: "invalid_account_token" }, { status: 400 });
  }

  const secret = Deno.env.get("TELEMETRY_HASH_SECRET");
  if (!secret) throw new Error("TELEMETRY_HASH_SECRET is not set");

  const accountTokenHash = await hmacSha256Hex(
    secret,
    payload.accountToken.trim(),
  );
  return Response.json({ accountTokenHash });
}
