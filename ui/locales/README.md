# Locales

The dashboard's strings live here, one file per language, as flat `key: value`
pairs with the same keys as `en.json`.

**Current state:** the page renders English inline, and these files are the
contract a translation follows. The lookup that reads them is not wired yet —
that is a roadmap item, not a claim about today. Translating a locale file now is
still useful work: it is the input that item needs.

Translations are welcome and **partial translations are accepted**: a key that is
missing falls back to English. To add a language:

1. Copy `en.json` to `<locale>.json`, for example `pt-BR.json`.
2. Translate the values. Do not translate the keys.
3. Open a pull request. A native speaker reviews it before merge.

Right-to-left languages (Arabic, Persian, Hebrew) additionally need a
`"dir": "rtl"` entry in the file; see [CONTRIBUTING.md](../../CONTRIBUTING.md).
