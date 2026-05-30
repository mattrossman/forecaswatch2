#!/usr/bin/env node

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const devConfigPath = path.join('src', 'pkjs', 'dev-config.js');
const stampPath = path.join('build', '.dev-config.stamp');
const bundlePath = path.join('build', 'pebble-js-app.js');

function hashDevConfig() {
  if (!fs.existsSync(devConfigPath)) {
    return 'missing';
  }
  const contents = fs.readFileSync(devConfigPath);
  return crypto.createHash('sha256').update(contents).digest('hex');
}

function readStamp() {
  if (!fs.existsSync(stampPath)) {
    return null;
  }
  return fs.readFileSync(stampPath, 'utf8').trim();
}

const nextHash = hashDevConfig();
const previousHash = readStamp();

if (nextHash !== previousHash) {
  fs.mkdirSync(path.dirname(stampPath), { recursive: true });
  fs.writeFileSync(stampPath, nextHash + '\n');
  if (fs.existsSync(bundlePath)) {
    fs.unlinkSync(bundlePath);
    const mapPath = bundlePath + '.map';
    if (fs.existsSync(mapPath)) {
      fs.unlinkSync(mapPath);
    }
  }
  console.log('dev-config.js changed; forcing PKJS rebundle');
}
