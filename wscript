#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
import os.path
import json
import re

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    """
    This method is used to configure your build. ctx.load(`pebble_sdk`) automatically configures
    a build for each valid platform in `targetPlatforms`. Platform-specific configuration: add your
    change after calling ctx.load('pebble_sdk') and make sure to set the correct environment first.
    Universal configuration: add your change prior to calling ctx.load('pebble_sdk').
    """
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    with open('package.json') as package_file:
        package = json.load(package_file)

    enable_memory_logging = os.environ.get('ENABLE_MEMORY_LOGGING', '').strip().lower() in ('1', 'true', 'yes', 'on')
    fixture_name = os.environ.get('FIXTURE', '').strip()
    fixture_now = None
    fixture_clock_24h = None
    fixture_battery = None
    if fixture_name:
        if not re.match(r'^[a-z0-9][a-z0-9-]*$', fixture_name):
            ctx.fatal('FIXTURE must be a fixture slug like "readme" or "rainy-night"')
        fixture_path = os.path.join('fixtures', '{}.json'.format(fixture_name))
        if not os.path.exists(fixture_path):
            ctx.fatal('Fixture not found: {}'.format(fixture_path))
        with open(fixture_path) as fixture_file:
            fixture = json.load(fixture_file)
        watch_fixture = fixture.get('watch', {})
        fixture_now = watch_fixture.get('now')
        if not isinstance(fixture_now, dict):
            ctx.fatal('Fixture {} must define watch.now'.format(fixture_path))
        for field in ('year', 'month', 'day', 'hour', 'minute', 'second'):
            if field not in fixture_now:
                ctx.fatal('Fixture {} must define watch.now.{}'.format(fixture_path, field))
            try:
                fixture_now[field] = int(fixture_now[field])
            except (TypeError, ValueError):
                ctx.fatal('Fixture watch.now.{} must be an integer'.format(field))
        if not (1 <= fixture_now['month'] <= 12):
            ctx.fatal('Fixture watch.now.month must be 1-12')
        if not (1 <= fixture_now['day'] <= 31):
            ctx.fatal('Fixture watch.now.day must be 1-31')
        if not (0 <= fixture_now['hour'] <= 23):
            ctx.fatal('Fixture watch.now.hour must be 0-23')
        if not (0 <= fixture_now['minute'] <= 59):
            ctx.fatal('Fixture watch.now.minute must be 0-59')
        if not (0 <= fixture_now['second'] <= 59):
            ctx.fatal('Fixture watch.now.second must be 0-59')
        watch_settings = fixture.get('watchSettings', {})
        time_format = watch_settings.get('timeFormat')
        if time_format:
            if time_format not in ('12h', '24h'):
                ctx.fatal('Fixture watchSettings.timeFormat must be "12h" or "24h"')
            fixture_clock_24h = '1' if time_format == '24h' else '0'
        battery_fixture = watch_fixture.get('battery')
        if battery_fixture is not None:
            if not isinstance(battery_fixture, dict):
                ctx.fatal('Fixture watch.battery must be an object')
            fixture_battery = {}
            if 'percent' not in battery_fixture:
                ctx.fatal('Fixture watch.battery.percent is required when watch.battery is defined')
            try:
                fixture_battery['percent'] = int(battery_fixture['percent'])
            except (TypeError, ValueError):
                ctx.fatal('Fixture watch.battery.percent must be an integer')
            if not (0 <= fixture_battery['percent'] <= 100):
                ctx.fatal('Fixture watch.battery.percent must be 0-100')
            charging = battery_fixture.get('charging', False)
            if isinstance(charging, bool):
                fixture_battery['charging'] = '1' if charging else '0'
            else:
                ctx.fatal('Fixture watch.battery.charging must be true or false')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        # Suppress SDK linker-script RWX segment noise.
        # Pebble's single APP region is expected: https://sourceware.org/binutils/docs/ld/Options.html#index-_002d_002dwarn_002drwx_002dsegments
        ctx.env.LINKFLAGS += ['-Wl,--no-warn-rwx-segments']
        if enable_memory_logging:
            ctx.env.CFLAGS += ['-DFCW2_ENABLE_MEMORY_LOGGING=1']
        if fixture_now:
            ctx.env.CFLAGS += [
                '-DFCW2_FIXTURE_NOW_YEAR={}'.format(fixture_now['year']),
                '-DFCW2_FIXTURE_NOW_MONTH={}'.format(fixture_now['month']),
                '-DFCW2_FIXTURE_NOW_DAY={}'.format(fixture_now['day']),
                '-DFCW2_FIXTURE_NOW_HOUR={}'.format(fixture_now['hour']),
                '-DFCW2_FIXTURE_NOW_MINUTE={}'.format(fixture_now['minute']),
                '-DFCW2_FIXTURE_NOW_SECOND={}'.format(fixture_now['second']),
            ]
        if fixture_clock_24h is not None:
            ctx.env.CFLAGS += ['-DFCW2_FIXTURE_CLOCK_24H={}'.format(fixture_clock_24h)]
        if fixture_battery is not None:
            ctx.env.CFLAGS += [
                '-DFCW2_FIXTURE_BATTERY_PERCENT={}'.format(fixture_battery['percent']),
                '-DFCW2_FIXTURE_BATTERY_CHARGING={}'.format(fixture_battery['charging']),
            ]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'), target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json',
                                         'src/common/**/*.js',
                                         'package.json',
                                         'release-notifications.json']),
                   js_entry_file='src/pkjs/index.js')
