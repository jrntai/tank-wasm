#!/usr/bin/env node
// Static web server for the game build. Specified in specs/11-web-server.md.
// No dependencies: node:http, node:fs and node:path only.
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

export const INDEX = 'tank.html';

// WEB-2
const TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.wasm': 'application/wasm',
  '.map': 'application/json; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.png': 'image/png',
  '.ico': 'image/x-icon',
};

// Maps a URL pathname to a file inside `root`, or null if it must not be served (WEB-3).
function resolveFile(root, pathname) {
  let decoded;
  try {
    decoded = decodeURIComponent(pathname);
  } catch {
    return null;  // malformed escape such as %zz
  }
  if (decoded.includes('\0')) return null;
  const rel = decoded === '/' ? INDEX : decoded.replace(/^\/+/, '');
  const segments = rel.split(/[/\\]/);
  if (segments.some((s) => s === '..' || s.startsWith('.'))) return null;
  const full = path.resolve(root, rel);
  if (!full.startsWith(root + path.sep)) return null;
  return full;
}

export function createServer({ root, quiet = false }) {
  root = path.resolve(root);
  const log = quiet ? () => {} : (req, status) => console.log(`${req.method} ${req.url} ${status}`);

  return http.createServer((req, res) => {
    res.setHeader('X-Content-Type-Options', 'nosniff');  // WEB-8
    const finish = (status, body = '', headers = {}) => {
      res.writeHead(status, { 'Content-Type': 'text/plain; charset=utf-8', ...headers });
      res.end(req.method === 'HEAD' ? undefined : body);
      log(req, status);
    };

    if (req.method !== 'GET' && req.method !== 'HEAD') {
      return finish(405, 'method not allowed\n', { Allow: 'GET, HEAD' });  // WEB-5
    }

    // Only the path matters; parse it without letting URL normalization resolve `..`.
    const rawPath = (req.url ?? '/').split('?')[0];
    if (rawPath === '/healthz') return finish(200, 'ok\n');  // WEB-7

    const file = resolveFile(root, rawPath);
    if (!file) return finish(404, 'not found\n');

    fs.stat(file, (err, stat) => {
      if (err || !stat.isFile()) return finish(404, 'not found\n');  // WEB-3, WEB-4

      // WEB-6: revalidate every time; the ETag changes whenever the file does.
      const etag = `"${stat.size.toString(16)}-${Math.floor(stat.mtimeMs).toString(16)}"`;
      const headers = {
        'Content-Type': TYPES[path.extname(file).toLowerCase()] ?? 'application/octet-stream',
        'Cache-Control': 'no-cache',
        ETag: etag,
      };
      if (req.headers['if-none-match'] === etag) {
        res.writeHead(304, headers);
        res.end();
        return log(req, 304);
      }

      res.writeHead(200, { ...headers, 'Content-Length': stat.size });
      if (req.method === 'HEAD') {
        res.end();
        return log(req, 200);
      }
      const stream = fs.createReadStream(file);
      stream.on('error', () => res.destroy());
      stream.pipe(res);
      log(req, 200);
    });
  });
}

// --- Command line (WEB-9) ---

function parseArgs(argv) {
  const here = path.dirname(fileURLToPath(import.meta.url));
  const opts = {
    dir: path.join(here, '..', 'build-web', 'web'),  // WEB-11
    port: process.env.PORT ?? '8080',
    host: process.env.HOST ?? '127.0.0.1',
  };
  for (let i = 0; i < argv.length; i++) {
    const flag = argv[i];
    const value = argv[i + 1];
    if (flag === '--dir') opts.dir = value;
    else if (flag === '--port') opts.port = value;
    else if (flag === '--host') opts.host = value;
    else if (flag === '--help' || flag === '-h') {
      console.log('usage: node server/server.mjs [--dir DIR] [--port N] [--host H]');
      process.exit(0);
    } else {
      console.error(`unknown argument: ${flag}`);
      process.exit(2);
    }
    i++;
  }
  opts.dir = path.resolve(opts.dir);
  opts.port = Number(opts.port);
  return opts;
}

function main() {
  const opts = parseArgs(process.argv.slice(2));
  if (!fs.existsSync(path.join(opts.dir, INDEX))) {
    console.error(`${path.join(opts.dir, INDEX)} not found. Build the game first:\n` +
                  '  emcmake cmake -S . -B build-web && cmake --build build-web');
    process.exit(1);
  }
  const server = createServer({ root: opts.dir });
  server.on('error', (err) => {
    console.error(err.code === 'EADDRINUSE'
      ? `port ${opts.port} on ${opts.host} is already in use; try --port`
      : `server error: ${err.message}`);
    process.exit(1);
  });
  server.listen(opts.port, opts.host, () => {
    const { port } = server.address();
    console.log(`Serving ${opts.dir} at http://${opts.host}:${port}/`);
  });
  for (const signal of ['SIGINT', 'SIGTERM']) {
    process.on(signal, () => server.close(() => process.exit(0)));
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) main();
