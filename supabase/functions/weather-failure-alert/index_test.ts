import { handleRequest } from "./handler.ts";

Deno.test("requires the default secret key", async () => {
  Deno.env.set("SUPABASE_URL", "http://localhost:54321");
  Deno.env.set("SUPABASE_PUBLISHABLE_KEY", "sb_publishable_test_key");
  Deno.env.set(
    "SUPABASE_SECRET_KEYS",
    JSON.stringify({ default: "sb_secret_test_key" }),
  );
  try {
    for (const key of [null, "sb_secret_wrong_key"]) {
      const headers = new Headers();
      if (key) headers.set("apikey", key);
      const response = await handleRequest(
        new Request("http://localhost", { method: "POST", headers }),
      );
      if (response.status !== 401) {
        throw new Error(`Expected 401, got ${response.status}`);
      }
    }
    const response = await handleRequest(
      new Request("http://localhost", {
        method: "GET",
        headers: { apikey: "sb_secret_test_key" },
      }),
    );
    if (response.status !== 405) {
      throw new Error(
        `Expected 405, got ${response.status}: ${await response.text()}`,
      );
    }
  } finally {
    Deno.env.delete("SUPABASE_URL");
    Deno.env.delete("SUPABASE_PUBLISHABLE_KEY");
    Deno.env.delete("SUPABASE_SECRET_KEYS");
  }
});
