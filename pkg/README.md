# pkg — native title sources

This folder is the title the console installs. `pandora package` archives it as
`<TITLEID>/` at the top of `dist/<TITLEID>.zip`.

| Path | Purpose |
| --- | --- |
| `sce_sys/param.json` | Title identity and `contentVersion`. Required. |
| `sce_sys/icon0.png` | 512×512 square icon shown on the home screen and in stores. Required. |
| `eboot.bin` | The payload binary. **Not present yet** — produced by the hardware build. |

## The title ID

`PPSA77010` is a **placeholder**. Before publishing anything, reserve your own
ID following [../docs/TITLE-ID.md](../docs/TITLE-ID.md). A title ID is the
app's permanent identity: changing it later makes the console treat the app as
a different title with separate save data.

## Raising `contentVersion`

Every release must raise `contentVersion`, or consoles will not see it as an
update. The tooling does it for you:

```sh
cargo run -p pandora-cli -- param bump pkg/sce_sys/param.json
```
