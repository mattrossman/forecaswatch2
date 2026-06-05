alter type "public"."weather_provider" rename to "weather_provider__old_version_to_be_dropped";

create type "public"."weather_provider" as enum ('wunderground', 'openweathermap', 'mock', 'dwd');

alter table "public"."telemetry_weather_fetch" alter column provider type "public"."weather_provider" using provider::text::"public"."weather_provider";

drop type "public"."weather_provider__old_version_to_be_dropped";


