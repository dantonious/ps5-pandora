# Releasing

Only maintainers publish releases. The checklist is short because each item is a
failure mode we have seen.

## Before you tag

1. **Raise `contentVersion`.** This is the step that gets forgotten, and the
   consequence is invisible: the release exists, users install it, and no
   console ever offers it as an update.

   ```sh
   ./build/pandora-cli param bump pkg/sce_sys/param.json
   ./build/pandora-cli param show pkg/sce_sys/param.json
   ```

   `--minor` and `--major` are also accepted. A bump clamps at the field width.

2. **Validate the metadata.**

   ```sh
   ./build/pandora-cli param validate pkg/sce_sys/param.json
   ```

3. **Run the gates.**

   ```sh
   make fmt-check && make test
   ```

4. **Update `CHANGELOG.md`.** Move entries out of `Unreleased` into a version
   heading. The changelog is what users read; commit messages are not.

5. **Test on hardware, or say you did not.** A release note that claims hardware
   support without a test report is a lie users will discover. Today the honest
   answer is that nothing has run on hardware yet.

## Build the artifact

```sh
make package
./build/pandora-cli digest dist/PPSA77010.zip
```

The tag must equal the version, and the asset must be named `<TITLEID>.zip`.
Record the SHA-256: it is what a catalog record pins.

## Publish

1. Create the GitHub release with tag `vX.Y.Z`.
2. Attach `dist/<TITLEID>.zip` — **the only installable file**. Do not attach
   `.elf` payloads or source archives to the same release; tooling that scans
   releases treats "several installable files" as ambiguous and skips the
   project.
3. Paste the changelog section as the release description.
4. **Never replace an asset of a release that is already published.** Its hash
   is pinned downstream. Publish a new release instead.

## Immutability

Enable GitHub's immutable releases for this repository. It turns "we promise not
to replace the file" into a guarantee.

## If you also list in a catalog

A catalog record pins the artifact's SHA-256, so the bytes you publish are the
bytes users get. The record also carries `titleId`, `version`, `artifact_url`
and an icon URL pinned to a tag rather than to `main` — an icon that can change
under a listing is an icon that can be swapped after review.

## After the release

- Announce it where the project is discussed.
- Thank the contributors whose work is in it, by name.
- Open a follow-up issue for anything you deferred.
