#!/usr/bin/env node
// Publish ChefZ_Wiki/ to the GitHub wiki repository.
//
//   node ChefZ_Wiki/publish.mjs [--dry-run]
//
// The wiki is a separate git repository that GitHub only creates once the wiki has
// been enabled and given a first page through the web interface. Until then this
// script stops and says so, rather than failing on an obscure git error.
// See PUBLISH.md.

import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const WIKI_DIR = path.dirname(fileURLToPath(import.meta.url));
const REPO = 'https://github.com/Psyern/Psyerns_ChefZ.wiki.git';

// Tooling, not wiki content.
const EXCLUDE = new Set(['PUBLISH.md', 'publish.mjs']);

const dryRun = process.argv.includes('--dry-run');

function git(args, opts = {}) {
  return execFileSync('git', args, { encoding: 'utf8', ...opts }).trim();
}

function fail(msg) {
  console.error('\n' + msg + '\n');
  process.exit(1);
}

// --- Does the wiki repository exist yet? ------------------------------------

try {
  execFileSync('git', ['ls-remote', REPO], { stdio: 'pipe' });
} catch {
  fail(
    `The wiki repository does not exist yet.\n\n` +
    `GitHub creates ${REPO}\n` +
    `only after the wiki has been enabled and given a first page in the browser:\n\n` +
    `  1. https://github.com/Psyern/Psyerns_ChefZ/settings  ->  tick "Wikis"\n` +
    `  2. https://github.com/Psyern/Psyerns_ChefZ/wiki      ->  create any first page\n\n` +
    `Then run this script again. It will replace that first page.`
  );
}

// --- Collect the pages ------------------------------------------------------

const pages = fs.readdirSync(WIKI_DIR)
  .filter(f => f.endsWith('.md') && !EXCLUDE.has(f))
  .sort();

if (pages.length === 0) fail('No pages found in ChefZ_Wiki/.');

const missing = ['Home.md', '_Sidebar.md'].filter(f => !pages.includes(f));
if (missing.length) fail(`Missing required page(s): ${missing.join(', ')}`);

console.log(`${pages.length} pages to publish:`);
for (const p of pages) console.log('  ' + p.replace(/\.md$/, ''));

if (dryRun) {
  console.log('\n--dry-run: nothing was cloned, written or pushed.');
  process.exit(0);
}

// --- Clone, replace, push ---------------------------------------------------

const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'chefz-wiki-'));
try {
  console.log(`\nCloning wiki into ${tmp}`);
  git(['clone', '--quiet', REPO, tmp]);

  // Warn about edits made in the browser: they are about to be overwritten.
  const log = git(['log', '--oneline', '-20'], { cwd: tmp });
  const foreign = log.split('\n').filter(l => l && !/Publish wiki from ChefZ_Wiki/.test(l));
  if (foreign.length) {
    console.log(
      `\nNote: the wiki has ${foreign.length} commit(s) that did not come from here.\n` +
      `They are about to be replaced. Most recent:\n  ` + foreign.slice(0, 3).join('\n  ') + '\n'
    );
  }

  for (const f of fs.readdirSync(tmp)) {
    if (f === '.git') continue;
    fs.rmSync(path.join(tmp, f), { recursive: true, force: true });
  }
  for (const p of pages) {
    fs.copyFileSync(path.join(WIKI_DIR, p), path.join(tmp, p));
  }

  git(['add', '-A'], { cwd: tmp });

  const status = git(['status', '--porcelain'], { cwd: tmp });
  if (!status) {
    console.log('The wiki is already up to date. Nothing to push.');
    process.exit(0);
  }

  const stamp = git(['log', '-1', '--format=%h'], { cwd: WIKI_DIR });
  git(['commit', '--quiet', '-m',
    `Publish wiki from ChefZ_Wiki (source ${stamp})`], { cwd: tmp });
  git(['push', '--quiet', 'origin', 'HEAD'], { cwd: tmp });

  console.log('\nPublished. https://github.com/Psyern/Psyerns_ChefZ/wiki');
} finally {
  fs.rmSync(tmp, { recursive: true, force: true });
}
