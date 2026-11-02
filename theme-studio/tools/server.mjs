import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { resolve, extname, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('../', import.meta.url));
const types = { '.html': 'text/html; charset=utf-8', '.mjs': 'text/javascript', '.js': 'text/javascript', '.css': 'text/css', '.lua': 'text/plain; charset=utf-8', '.cw': 'text/plain; charset=utf-8', '.json': 'application/json', '.svg': 'image/svg+xml' };
const port = Number(process.env.PORT || 4317);
types['.png']='image/png';
http.createServer(async (req, res) => {
  try {
    const pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
    const path = resolve(root, '.' + (pathname === '/' ? '/studio/index.html' : pathname));
    if (!path.startsWith(root.endsWith(sep) ? root : root + sep) || /(^|[\\/])\./.test(path.slice(root.length))) {
      res.writeHead(403).end(); return;
    }
    const body = await readFile(path);
    res.writeHead(200, { 'Content-Type': types[extname(path)] || 'application/octet-stream', 'Cache-Control': 'no-store' });
    res.end(body);
  } catch { res.writeHead(404).end('Not found'); }
}).listen(port, '127.0.0.1', () => console.log(`Theme Studio: http://127.0.0.1:${port}/`));
