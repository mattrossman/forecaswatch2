import { handleRequest } from "./handler.ts";

function assertEquals(actual: unknown, expected: unknown) {
  if (actual !== expected) {
    throw new Error(`Expected ${expected}, got ${actual}`);
  }
}

Deno.test("hashes the trimmed token with the ingestion secret", async () => {
  Deno.env.set("TELEMETRY_HASH_SECRET", "key");
  const response = await handleRequest(
    new Request("http://localhost", {
      method: "POST",
      body: JSON.stringify({
        accountToken: "  The quick brown fox jumps over the lazy dog  ",
      }),
    }),
  );
  assertEquals(response.status, 200);
  const body = await response.json();
  assertEquals(
    body.accountTokenHash,
    "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
  );
});

Deno.test("rejects invalid input and methods", async () => {
  for (
    const body of [
      "{",
      "null",
      "{}",
      '{"accountToken":"  "}',
      '{"accountToken":1}',
    ]
  ) {
    const response = await handleRequest(
      new Request("http://localhost", { method: "POST", body }),
    );
    assertEquals(response.status, 400);
  }
  const response = await handleRequest(new Request("http://localhost"));
  assertEquals(response.status, 405);
});
