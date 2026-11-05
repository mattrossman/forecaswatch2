import { z } from "npm:zod@4.3.6";

export const accountTokenSchema = z.string().trim().min(1, {
  message: "invalid_account_token",
});
