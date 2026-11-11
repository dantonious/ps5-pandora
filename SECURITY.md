# Security policy

## Reporting a vulnerability

**Do not open a public issue for a security problem.**

Report it privately through GitHub's
[private vulnerability reporting](https://github.com/dantonious/ps5-pandora/security/advisories/new),
or by email to `security@ps5-pandora.dev`.

Include:

- What the problem is and where (crate, file, endpoint).
- How to reproduce it, or a proof of concept.
- The impact you believe it has.
- The version, commit, and whether it was observed on hardware or on the host.

We will acknowledge within **5 working days**, tell you whether we consider it
a vulnerability, and keep you informed until it is fixed. We will credit you in
the advisory unless you prefer otherwise.

Please give us **90 days** before public disclosure, or less if a fix ships
sooner. If a vulnerability is being exploited in the wild, say so and we will
move faster.

## What counts as a vulnerability here

This software runs with high privilege on a console, so the bar is low and the
consequences are high. In scope:

- Memory-safety bugs in safe Rust code, or unsound `unsafe` in
  `platform::ps5` — including a `// SAFETY:` comment that is simply wrong.
- Path traversal or arbitrary file write through the HTTP API, especially
  anything that escapes the configured data root.
- Authentication or authorisation bypass on the dashboard.
- Remotely triggerable panic or denial of service.
- Archive extraction that writes outside the destination (zip-slip).
- Leaking credentials, tokens or console identifiers in logs or responses.

## Out of scope

- The jailbreak itself, or anything requiring an already-compromised console
  in a way the project does not control.
- Vulnerabilities in Sony's firmware or in the PS5 Payload SDK — report those
  upstream. Tell us anyway if the impact reaches this project.
- Findings that require physical access to the console.
- Missing hardening headers on a service that only listens on localhost, unless
  you can show a concrete attack.
- Reports produced solely by a scanner, with no demonstrated impact.

## Supported versions

While the project is pre-1.0, only the latest release receives fixes.
