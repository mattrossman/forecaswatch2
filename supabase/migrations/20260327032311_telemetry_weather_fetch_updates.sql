alter table "public"."telemetry_weather_fetch" add column "attempt" integer;

alter table "public"."telemetry_weather_fetch" add column "gps_error_code" integer;

alter table "public"."telemetry_weather_fetch" add column "location_mode" text;

alter table "public"."telemetry_weather_fetch" add column "used_gps_cache" boolean not null default false;

alter table "public"."telemetry_weather_fetch" add constraint "telemetry_weather_fetch_attempt_check" CHECK ((attempt >= 1)) not valid;

alter table "public"."telemetry_weather_fetch" validate constraint "telemetry_weather_fetch_attempt_check";

alter table "public"."telemetry_weather_fetch" add constraint "telemetry_weather_fetch_location_mode_check" CHECK (((location_mode IS NULL) OR (location_mode = ANY (ARRAY['gps'::text, 'manual_coordinates'::text, 'manual_address'::text])))) not valid;

alter table "public"."telemetry_weather_fetch" validate constraint "telemetry_weather_fetch_location_mode_check";


