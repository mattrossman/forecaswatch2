CREATE INDEX telemetry_weather_fetch_received_at_idx ON public.telemetry_weather_fetch USING btree (received_at DESC);
