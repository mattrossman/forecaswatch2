alter table "public"."telemetry_weather_fetch" add column "gps_error_code" integer;

alter table "public"."telemetry_weather_fetch" add column "used_gps_cache" boolean not null default false;


