# Governance

ps5-pandora is a small volunteer project. This document exists so that
decisions do not depend on guessing who is in charge.

## Roles

**Contributor** — anyone who opens an issue or pull request. No commitment
required, no special access.

**Reviewer** — a contributor with a track record of good changes in an area,
who can review and approve pull requests there. Added by a maintainer after
a handful of merged contributions. Reviewers can approve; they cannot merge
changes to `platform::ps5`, the release pipeline, or licensing.

**Maintainer** — merge rights, release rights, and responsibility for the
project's direction. Maintainers are listed in `.github/CODEOWNERS`. A
maintainer is added when the existing maintainers agree unanimously, after the
person has reviewed substantially for a while.

**Anyone can leave at any time.** Access is revoked on request, no explanation
needed.

## How decisions are made

**Lazy consensus.** Most decisions need no vote: someone proposes, nobody
objects within a reasonable window, it happens. Silence is agreement for
routine work.

**Escalation.** When there is disagreement, the maintainers decide by simple
majority. The reasoning is written in the issue so the decision can be
understood later, even if it is revisited.

**Reversibility.** Decisions that are cheap to reverse (a naming choice, a
default) are made quickly by the person doing the work. Decisions that are
expensive to reverse (the crate split, the licence, the persistence format)
get an issue and a discussion first.

**Architecture decisions** that are expensive to reverse are recorded as short
notes under `docs/decisions/` (`NNNN-title.md`: context, decision, consequences).
There is no process for the sake of process — a decision record is written when
future-you would otherwise ask "why on earth is it like this?".

## Releases

Only maintainers publish releases. A release follows
[docs/RELEASING.md](docs/RELEASING.md). Version numbers follow
[Semantic Versioning](https://semver.org/); while the project is pre-1.0, minor
versions may break compatibility, and the changelog says so.

## Conflicts of interest

If a decision affects a project you maintain elsewhere, say so in the issue and
let the other maintainers decide. This matters here: some contributors also work
on related console projects.

## Code of conduct

Enforcement is described in [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md). A
maintainer who is the subject of a report does not participate in deciding it.

## Changing this document

Open a pull request. Governance changes need two maintainer approvals.
