# ESP Web Tools 10.1.0

Upstream: https://github.com/esphome/esp-web-tools/tree/10.1.0

Distribution: https://registry.npmjs.org/esp-web-tools/-/esp-web-tools-10.1.0.tgz

The files in this directory are the upstream `dist/web` ES modules, with the
Apache-2.0 license retained as `LICENSE`. The upstream package exposes no locale
API; its `overrides` option only supports `checkSameFirmware`.

Local changes are limited to UI localization:

- `localization.mjs` holds English-to-Chinese text, localized Lit template
  caching, and language-change listeners. It reads the portal's `i18n.mjs`.
- `install-button.js` localizes its fallback Connect label and alert content.
- `index-BaUGegDD.js` uses localized templates for the no-port-selected dialog.
- `install-dialog-CQzvAv3p.js` uses localized templates and explicit UI string
  translation for installation, erase confirmation, progress, Wi-Fi, errors,
  and console controls. Manifest names, network credentials, URLs, and device
  data retain their original values. Serial flashing logic is unchanged.

When updating the pinned upstream version, recheck the three changed entry
chunks against upstream and update all new UI messages in `localization.mjs`.
Check both languages, including cancellation of the native port chooser,
erase confirmation, installation confirmation, progress, and failure states.

The browser-owned Web Serial permission dialog and port chooser use the
browser/operating system language. This customization only translates the
website and ESP Web Tools dialogs; it does not control browser UI.
