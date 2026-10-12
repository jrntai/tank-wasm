// Tests for specs/11-web-server.md. Each test is named after its criterion ID.
import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { createServer } from '../server.mjs';

const SERVER = fileURLToPath(new URL('../server.mjs', import.meta.url));

// A fake build: page, script, wasm, plus things that must never be served.
const sandbox = fs.mkdtempSync(path.join(os.tmpdir(), 'tank-web-'));
const root = path.join(sandbox, 'build-web');
const WASM = Buffer.from([0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]);
fs.mkdirSync(path.join(root, 'CMakeFiles'), { recursive: true });
fs.writeFileSync(path.join(root, 'tank.html'), '<!doctype html><title>tank</title>');
fs.writeFileSync(path.join(root, 'tank.js'), 'console.log("tank")');
fs.writeFileSync(path.join(root, 'tank.wasm'), WASM);
fs.writeFileSync(path.join(root, 'notes.xyz'), 'x');
fs.writeFileSync(path.join(root, '.secret'), 'dotfile-secret');
fs.writeFileSync(path.join(root, 'CMakeFiles', 'inner.txt'), 'inner');
fs.writeFileSync(path.join(sandbox, 'outside.txt'), 'outside-secret');

let server;
let port;

before(async () => {
  server = createServer({ root, quiet: true });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  port = server.address().port;
});

after(async () => {
  await new Promise((resolve) => server.close(resolve));
  fs.rmSync(sandbox, { recursive: true, force: true });
});

// Raw request, so paths reach the server exactly as written (fetch would normalize `..`).
function request(method, rawPath, headers = {}, p = port) {
  return new Promise((resolve, reject) => {
    const req = http.request({ host: '127.0.0.1', port: p, method, path: rawPath, headers }, (res) => {
      const chunks = [];
      res.on('data', (c) => chunks.push(c));
      res.on('end', () => resolve({ status: res.statusCode, headers: res.headers, body: Buffer.concat(chunks) }));
    });
    req.on('error', reject);
    req.end();
  });
}

test('WEB-1 GET / serves tank.html', async () => {
  const r = await request('GET', '/');
  assert.equal(r.status, 200);
  assert.equal(r.body.toString(), '<!doctype html><title>tank</title>');
});

test('WEB-2 content types and lengths', async () => {
  const cases = [
    ['/tank.html', 'text/html; charset=utf-8'],
    ['/tank.js', 'text/javascript; charset=utf-8'],
    ['/tank.wasm', 'application/wasm'],
    ['/notes.xyz', 'application/octet-stream'],
  ];
  for (const [p, type] of cases) {
    const r = await request('GET', p);
    assert.equal(r.status, 200, p);
    assert.equal(r.headers['content-type'], type, p);
    assert.equal(Number(r.headers['content-length']), r.body.length, p);
  }
  const wasm = await request('GET', '/tank.wasm');
  assert.deepEqual(wasm.body, WASM);
});

test('WEB-3 traversal, dotfiles and directories are refused', async () => {
  const attempts = [
    '/../outside.txt',
    '/%2e%2e/outside.txt',
    '/..%2foutside.txt',
    '/%2e%2e%2foutside.txt',
    '/CMakeFiles/..%2f..%2foutside.txt',
    '/%2Fetc%2Fpasswd',
    '//etc/passwd',
    '/tank.html%00.js',
    '/.secret',
    '/%2esecret',
    '/CMakeFiles',
    '/CMakeFiles/',
    '/%zz',
  ];
  for (const p of attempts) {
    const r = await request('GET', p);
    assert.equal(r.status, 404, p);
    const body = r.body.toString();
    assert.ok(!body.includes('outside-secret') && !body.includes('dotfile-secret') && !body.includes('root:'), p);
  }
  // Files in subdirectories are fine when the path is clean.
  assert.equal((await request('GET', '/CMakeFiles/inner.txt')).status, 200);
});

test('WEB-4 missing paths are 404', async () => {
  assert.equal((await request('GET', '/nope.js')).status, 404);
  assert.equal((await request('GET', '/a/b/c')).status, 404);
});

test('WEB-5 HEAD has headers without body; other methods 405', async () => {
  const get = await request('GET', '/tank.js');
  const head = await request('HEAD', '/tank.js');
  assert.equal(head.status, 200);
  assert.equal(head.body.length, 0);
  assert.equal(head.headers['content-length'], get.headers['content-length']);
  assert.equal(head.headers['content-type'], get.headers['content-type']);
  for (const m of ['POST', 'PUT', 'DELETE', 'PATCH']) {
    const r = await request(m, '/tank.js');
    assert.equal(r.status, 405, m);
    assert.equal(r.headers.allow, 'GET, HEAD', m);
  }
});

test('WEB-6 no-cache with ETag; 304 on match; ETag changes with the file', async () => {
  const first = await request('GET', '/tank.js');
  assert.equal(first.headers['cache-control'], 'no-cache');
  const etag = first.headers.etag;
  assert.ok(etag);
  const again = await request('GET', '/tank.js', { 'If-None-Match': etag });
  assert.equal(again.status, 304);
  assert.equal(again.body.length, 0);

  const file = path.join(root, 'tank.js');
  fs.writeFileSync(file, 'console.log("rebuilt, longer")');
  const later = new Date(Date.now() + 5000);
  fs.utimesSync(file, later, later);
  const rebuilt = await request('GET', '/tank.js', { 'If-None-Match': etag });
  assert.equal(rebuilt.status, 200);
  assert.notEqual(rebuilt.headers.etag, etag);
});

test('WEB-7 /healthz returns ok', async () => {
  const r = await request('GET', '/healthz');
  assert.equal(r.status, 200);
  assert.equal(r.body.toString().trim(), 'ok');
});

test('WEB-8 nosniff on every response', async () => {
  for (const [m, p] of [['GET', '/'], ['GET', '/missing'], ['GET', '/../x'], ['POST', '/'], ['GET', '/healthz']]) {
    const r = await request(m, p);
    assert.equal(r.headers['x-content-type-options'], 'nosniff', `${m} ${p}`);
  }
});

// Runs the CLI until it prints its URL (or exits). Resolves with the process and output.
function startCli(args, env = {}) {
  return new Promise((resolve, reject) => {
    const child = spawn(process.execPath, [SERVER, ...args], { env: { ...process.env, ...env } });
    let out = '';
    let err = '';
    const timer = setTimeout(() => { child.kill(); reject(new Error(`timeout: ${out} ${err}`)); }, 5000);
    child.stdout.on('data', (d) => {
      out += d;
      const m = out.match(/Serving (.+) at (http:\/\/[^\s]+)/);
      if (m) { clearTimeout(timer); resolve({ child, dir: m[1], url: m[2], out, err }); }
    });
    child.stderr.on('data', (d) => { err += d; });
    child.on('exit', (code) => { clearTimeout(timer); resolve({ child, code, out, err }); });
  });
}

test('WEB-9 CLI defaults to 127.0.0.1, honors --dir/--port, fails clearly without a build', async () => {
  const run = await startCli(['--dir', root, '--port', '0']);
  try {
    assert.ok(run.url, run.out + run.err);
    const url = new URL(run.url);
    assert.equal(url.hostname, '127.0.0.1');
    assert.notEqual(url.port, '0');
    assert.equal(run.dir, root);
    const r = await request('GET', '/', {}, Number(url.port));
    assert.equal(r.status, 200);
  } finally {
    run.child.kill();
  }

  const missing = await startCli(['--dir', path.join(sandbox, 'nope'), '--port', '0']);
  assert.equal(missing.code, 1);
  assert.match(missing.err, /tank\.html not found/);
  assert.match(missing.err, /cmake --build build-web/);
});

test('WEB-11 the default directory is build-web/web', async () => {
  // Works whether or not the game has been built: either it serves that folder,
  // or it fails pointing at that folder.
  const run = await startCli(['--port', '0']);
  try {
    const shown = run.dir ?? run.err;
    assert.match(shown, /build-web[\\/]web/, run.out + run.err);
    if (run.url) {
      const port = Number(new URL(run.url).port);
      assert.equal((await request('GET', '/CMakeCache.txt', {}, port)).status, 404);
      assert.equal((await request('GET', '/compile_commands.json', {}, port)).status, 404);
    }
  } finally {
    run.child.kill();
  }
});
