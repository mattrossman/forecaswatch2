'use strict';

const fs = require('fs');

/**
 * Read and parse a JSON file.
 * @param {string} filePath Absolute or relative path to JSON file.
 * @returns {Object} Parsed JSON object.
 */
function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

/**
 * True when the PR appears to be managed by release-please.
 * @param {Object} event GitHub pull_request event payload.
 * @returns {boolean} Whether release notification enforcement should run.
 */
function isReleasePleasePr(event) {
  const pr = event.pull_request || {};
  const labels = Array.isArray(pr.labels) ? pr.labels.map((label) => label.name) : [];
  const headRef = String((pr.head && pr.head.ref) || '');

  const hasAutoreleaseLabel = labels.some((name) =>
    typeof name === 'string' && name.startsWith('autorelease:')
  );

  return Boolean(hasAutoreleaseLabel || headRef.startsWith('release-please--'));
}

/**
 * Validate required release notification fields for a version key.
 * @param {Object} notifications release-notifications.json object.
 * @param {string} version Version string from package.template.json.
 * @returns {boolean} True when entry exists with non-empty title/body.
 */
function hasValidNotification(notifications, version) {
  const entry = notifications && notifications[version];
  const title = entry && typeof entry.title === 'string' ? entry.title.trim() : '';
  const body = entry && typeof entry.body === 'string' ? entry.body.trim() : '';
  return Boolean(title && body);
}

function main() {
  const eventPath = process.env.GITHUB_EVENT_PATH;
  if (!eventPath) {
    throw new Error('GITHUB_EVENT_PATH is not set');
  }

  const event = readJson(eventPath);

  if (!isReleasePleasePr(event)) {
    console.log('Not a release-please PR; skipping enforcement.');
    return;
  }

  const pkg = readJson('package.template.json');
  const notifications = readJson('release-notifications.json');

  const version = String(pkg.version || '').trim();
  if (!version) {
    throw new Error('package.template.json must contain a non-empty "version"');
  }

  if (!hasValidNotification(notifications, version)) {
    throw new Error(
      `Missing or invalid release notification for ${version}. ` +
      `Add release-notifications.json["${version}"] with non-empty "title" and "body".`
    );
  }

  console.log(`Release notification is valid for ${version}.`);
}

try {
  main();
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
