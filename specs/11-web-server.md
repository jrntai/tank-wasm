# 11 — Web server

## Behaviour

`server/server.mjs` serves the web build over HTTP. CMake writes the game's output (`tank.html`, `tank.js`, `tank.wasm`, plus a source map in Debug) to its own folder, **`build-web/web/`**. The server serves only that folder, so CMake's internal files elsewhere in `build-web/` (`CMakeCache.txt`, `compile_commands.json` and others, which contain local paths) are never reachable. It's a single Node file with no dependencies, using only `node:http`, `node:fs` and `node:path`. It exports `createServer({ root, quiet })` for tests and runs as a command-line program otherwise.

**Routes**
- `GET /` serves `tank.html`, the page Emscripten generates.
- `GET /<file>` serves that file from the build directory: `tank.js`, `tank.wasm`, source maps and so on.
- `GET /healthz` returns `200 ok`, for scripts and uptime checks.
- Everything else returns `404`.

Only `GET` and `HEAD` are allowed; other methods get `405` with `Allow: GET, HEAD`. `HEAD` returns the same headers as `GET` with no body.

**Content types**

| Extension | Type |
|---|---|
| `.html` | `text/html; charset=utf-8` |
| `.js` | `text/javascript; charset=utf-8` |
| `.wasm` | `application/wasm`, required for streaming WebAssembly compilation |
| `.map`, `.json` | `application/json; charset=utf-8` |
| `.css` | `text/css; charset=utf-8` |
| `.png` | `image/png` |
| `.ico` | `image/x-icon` |
| anything else | `application/octet-stream` |

**Safety.** The server only ever serves regular files inside the build directory.
- Requests that decode to a path outside it are refused with `404`. That covers `..` segments, encoded slashes, absolute paths and NUL bytes.
- Directories get `404`.
- Any path segment starting with `.` gets `404`, so files like `.cmake` or `.git` are never served.

Every response carries `X-Content-Type-Options: nosniff`.

**Caching.** Files are sent with `Cache-Control: no-cache` and an `ETag` built from size and modification time. A matching `If-None-Match` gets `304` with no body. Browsers revalidate on every load, so a rebuild shows up on reload without stale WASM.

**Command line.** `node server/server.mjs [--dir DIR] [--port N] [--host H]`. The `PORT` and `HOST` environment variables also work; flags win. Defaults: `build-web/web/` next to the `server/` folder, port **8080**, host **127.0.0.1** (this machine only; pass `--host 0.0.0.0` to share on a LAN).
- If `DIR/tank.html` doesn't exist, it prints how to build and exits with status 1.
- If the port is taken, it says so and exits with status 1.
- On start, it prints `Serving <dir> at http://<host>:<port>/`.
- It logs one line per request (`METHOD path status`).

## Acceptance criteria

Tests: `server/test/server.test.mjs`, run with `node --test`. Each test is named after its ID.

- [x] **WEB-1** `GET /` returns `tank.html` with status 200.
- [x] **WEB-2** `.html`, `.js` and `.wasm` files are served with the content types above and a correct `Content-Length`. Unknown extensions get `application/octet-stream`.
- [x] **WEB-3** Traversal (`..`, encoded `%2e%2e` or `%2f`, NUL bytes), dotfiles and directories all get 404 and never leak file contents.
- [x] **WEB-4** Paths that don't exist get 404.
- [x] **WEB-5** `HEAD` returns the `GET` headers with no body. Other methods get 405 with `Allow: GET, HEAD`.
- [x] **WEB-6** Responses carry `Cache-Control: no-cache` and an `ETag`. A matching `If-None-Match` gets 304, and the `ETag` changes when the file changes.
- [x] **WEB-7** `GET /healthz` returns 200 `ok`.
- [x] **WEB-8** Every response, including errors, has `X-Content-Type-Options: nosniff`.
- [x] **WEB-9** The command line binds to 127.0.0.1 by default, honors `--port` and `--dir`, prints its URL, and exits with status 1 and a build hint when `tank.html` is missing.
- [~] **WEB-10** The game loads and plays in a browser from `http://127.0.0.1:8080/`. *(manual)*
- [x] **WEB-11** The web build's output goes to `build-web/web/`, which is the command line's default directory, so build internals such as `CMakeCache.txt` can't be fetched.

## Verification log

- 2026-10-12: `npm start` served `build-web/web/`. Headless Chrome loaded `http://127.0.0.1:8080/` and was driven with real key events: title, controls, stage select, stage 3 play (WEB-10). curl confirmed `application/wasm`, `/healthz`, and 404 for `/CMakeCache.txt` and `/compile_commands.json`.
