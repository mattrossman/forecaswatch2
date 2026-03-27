alter table "public"."telemetry_weather_fetch" add column "location_mode" text;

alter table "public"."telemetry_weather_fetch" add constraint "telemetry_weather_fetch_location_mode_check" CHECK (((location_mode IS NULL) OR (location_mode = ANY (ARRAY['gps'::text, 'manual_coordinates'::text, 'manual_address'::text])))) not valid;

alter table "public"."telemetry_weather_fetch" validate constraint "telemetry_weather_fetch_location_mode_check";


