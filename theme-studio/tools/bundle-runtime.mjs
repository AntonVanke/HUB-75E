import { build } from 'esbuild';
import {copyFile} from 'node:fs/promises';
await build({entryPoints:['studio/theme-package.mjs'],bundle:true,platform:'browser',format:'esm',outfile:'studio/vendor/package.mjs',minify:true});
await build({ entryPoints: ['node_modules/fengari/src/fengari.js'], bundle: true,
  platform: 'browser', format: 'iife', globalName: 'fengari',
  outfile: 'studio/vendor/fengari.js', minify: true, legalComments: 'eof',
  define: { 'process.env.FENGARICONF': 'undefined', process: 'undefined' } });
await copyFile('node_modules/fengari/LICENSE','studio/vendor/FENGARI-LICENSE.txt');
