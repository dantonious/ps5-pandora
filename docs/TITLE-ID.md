# Title IDs

A title ID is the app's permanent identity: the folder name on the console, the
`titleId` in `sce_sys/param.json`, and the filename of the catalog record. It is
four uppercase letters followed by five digits.

## Rules

- Exactly `^[A-Z]{4}[0-9]{5}$`.
- **Not `CUSA`** — that prefix belongs to PS4 titles.
- **Not `NPXS`** — that prefix belongs to system titles, and one of the loader
  tools skips such folders entirely.
- **Not `PPSA99999`** — the native-app boilerplate's default. Using it means your
  app collides with every other project that never changed it.
- No digits among the letters (`F4K377031` is rejected by tooling that parses
  the prefix).
- Case matters. Lowercase is not accepted.

`pandora` enforces all of this:

```sh
cargo run -p pandora-cli -- title-id check PPSA77010
```

## Choosing one

`PPSA` is the conventional prefix and what most projects use. Custom prefixes
work but may not be displayed by older store clients.

Check that nobody else has taken it before you build around it: title IDs are
first come, first served in the public homebrew catalog.

**`PPSA77010` in this repository is a placeholder.** Replace it before the first
public release.

## Changing it later is expensive

The console treats a different title ID as a **different app**: separate install
location, separate save data, separate update history. Users lose their data by
proxy. Treat the ID as immutable once released.

## Reserved IDs

If you are building something that is not released yet, you can reserve an ID in
the catalog before you have a build, so nobody else takes it. A reservation
expires if it is not refreshed, so it is a claim, not a permanent lock.

## Related

- [RELEASING.md](RELEASING.md) — the release checklist.
- `pkg/README.md` — where the ID lives in this repository.
