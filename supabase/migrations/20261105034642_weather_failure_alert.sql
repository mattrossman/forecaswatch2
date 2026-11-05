SET local check_function_bodies = off;

CREATE EXTENSION "pg_cron";

CREATE EXTENSION "pg_net" SCHEMA "extensions";

COMMENT ON EXTENSION "pg_cron" IS 'Job scheduler for PostgreSQL';

COMMENT ON EXTENSION "pg_net" IS 'Async HTTP';

SELECT cron.schedule_in_database('weather-failure-alert', '*/15 * * * *', '
    select net.http_post(
      url := (select decrypted_secret from vault.decrypted_secrets where name = ''weather_alert_project_url'') || ''/functions/v1/weather-failure-alert'',
      headers := jsonb_build_object(
        ''Content-Type'', ''application/json'',
        ''apikey'', (select decrypted_secret from vault.decrypted_secrets where name = ''weather_alert_secret_key'')
      ),
      body := ''{}''::jsonb
    );
  ', 'postgres', NULL, true);
