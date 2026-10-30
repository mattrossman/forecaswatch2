#!/usr/bin/env node

const fs = require('node:fs');

const MEMORY_BLOCK_PATTERN =
  /^(?<platform>[A-Z0-9_]+) APP MEMORY USAGE\s*\r?\nTotal size of resources:\s*(?<resources>[\d,]+) bytes \/ (?<resourcesCap>[^\r\n]+)\r?\nTotal footprint in RAM:\s*(?<ram>[\d,]+) bytes \/ (?<ramCap>[^\r\n]+)\r?\nFree RAM available \(heap\):\s*(?<free>[\d,]+) bytes$/gm;

/**
 * Parse Pebble SDK memory-usage blocks from build output.
 *
 * @param {string} logText Build log text.
 * @returns {Array<{
 *   platform: string,
 *   resources: string,
 *   resourcesCap: string,
 *   ram: string,
 *   ramCap: string,
 *   free: string,
 * }>} Parsed memory entries in log order.
 */
function parsePebbleMemoryReport(logText) {
  const entries = [];

  for (const match of logText.matchAll(MEMORY_BLOCK_PATTERN)) {
    const groups = match.groups;

    if (!groups) {
      continue;
    }

    entries.push({
      platform: groups.platform,
      resources: groups.resources,
      resourcesCap: groups.resourcesCap.trim(),
      ram: groups.ram,
      ramCap: groups.ramCap.trim(),
      free: groups.free,
    });
  }

  return entries;
}

/**
 * Convert a Pebble platform name into a human-friendly label.
 *
 * @param {string} platform Pebble platform identifier.
 * @returns {string} Human-friendly label.
 */
function humanizePlatform(platform) {
  return platform.charAt(0) + platform.slice(1).toLowerCase();
}

/**
 * Format a byte count for markdown output.
 *
 * @param {string} value Raw byte count from the build log.
 * @returns {string} Human-friendly byte count.
 */
function formatBytes(value) {
  return Number(value.replace(/,/g, '')).toLocaleString('en-US');
}

/**
 * Calculate the percentage of heap remaining from the RAM footprint and free heap.
 *
 * @param {string} ram RAM footprint in bytes.
 * @param {string} free Free heap in bytes.
 * @returns {string} Percentage of heap remaining with one decimal place.
 */
function formatFreeHeapPercent(ram, free) {
  const ramBytes = Number(ram.replace(/,/g, ''));
  const freeBytes = Number(free.replace(/,/g, ''));
  const heapBytes = ramBytes + freeBytes;

  if (!heapBytes) {
    return '0.0%';
  }

  return `${((freeBytes / heapBytes) * 100).toFixed(1)}%`;
}

/**
 * Format parsed Pebble memory entries as a markdown table.
 *
 * @param {Array<{
 *   platform: string,
 *   resources: string,
 *   resourcesCap: string,
 *   ram: string,
 *   ramCap: string,
 *   free: string,
 * }>} entries Parsed memory entries.
 * @returns {string} Markdown table or a fallback note.
 */
function renderPebbleMemoryReport(entries) {
  if (!entries.length) {
    return '_No memory usage blocks were found in the build log._';
  }

  const lines = [
    '### Memory usage',
    '',
    '| Platform | Resources | RAM footprint | Free heap |',
    '| --- | ---: | ---: | ---: |',
  ];

  for (const entry of entries) {
    lines.push(
      `| ${humanizePlatform(entry.platform)} | ${formatBytes(entry.resources)} bytes / ${entry.resourcesCap} | ${formatBytes(entry.ram)} bytes / ${entry.ramCap} | ${formatBytes(entry.free)} bytes (${formatFreeHeapPercent(entry.ram, entry.free)} free) |`
    );
  }

  lines.push('', `Parsed ${entries.length} platform${entries.length === 1 ? '' : 's'} from the build log.`);

  return lines.join('\n');
}

if (require.main === module) {
  const inputPath = process.argv[2];

  if (!inputPath) {
    console.error('Usage: parse-pebble-memory-report.js <build-log-path>');
    process.exit(1);
  }

  const logText = fs.readFileSync(inputPath, 'utf8');
  process.stdout.write(renderPebbleMemoryReport(parsePebbleMemoryReport(logText)));
}

module.exports = {
  humanizePlatform,
  parsePebbleMemoryReport,
  formatFreeHeapPercent,
  renderPebbleMemoryReport,
};
