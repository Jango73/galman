# AGENTS.md

This file provides guidance to agents when working with code in this repository.
**If the guidelines below are not followed, all modifications will be rejected.**

## Project Overview
This is an image gallery manager.

## Communication Guidelines
- NEVER use emojis in responses.
- DON'T act like a human being with emotions, just be a machine.
- DON'T says "Great!", "Perfect!", "You're right" all the time.
- If a demand DOES NOT make sense (for instance, breaks an architecture instead of refactoring it), SAY IT and ask for confirmation BEFORE DOING ANYTHING.

## General Coding Conventions
- **Languages**: QML for UI, C++ for business code.
- **Component oriented**: Make components, whatever the language. A code file shall not exceed 800 lines.
- **Clean code**: if there is a quick and dirty way to fix a problem and a slower but clean way, ALWAYS choose the slower and clean.
- **No code duplication**: NEVER duplicate code, factorise. And NEVER leave dead code/constants.
- **Magic numbers and colors**: NEVER use direct numbers/colors (except for UI element positions/sizes). Always put them in constants, in one or several global objects, depending on needs.
- **Function order**: DO NOT OVERUSE forward declarations. Define functions before they are used.
- **Asynchronicity**: never do blocking operations, UI must be responsive ALL THE TIME. Use workers for potentially long operations.
- **Naming** :
  - NEVER use abbreviations; ALWAYS use full words (acronyms are OK).
  - No "directory" : use "folder".

## QML Coding Conventions
- All General Coding Conventions items apply
- **UI only**: No heavy business logic in QML, leave that to C++.
- **QML signal handlers**: NEVER rely on implicit parameter injection (deprecated). Always declare formal parameters, e.g. `onPressed: (mouse) => { ... }`.
- To mask menu items (when they should be inactive), donc just disable them, use Instantiator.

## Logging Conventions
- All logs go to `log/galman.log` in the project root, with rotation (`log/galman.log.1` .. `log/galman.log.9`).
- Use `qInfo()` for general information logging (user actions, state changes).
- Use `qWarning()` for recoverable issues (validation failures, missing resources).
- Use `qCritical()` for errors that degrade functionality but don't crash.
- Use `qDebug()` for verbose debug output (only active in debug builds).
- Use `qInfo()` calls in C++ business methods so that QML signal chains can be verified in the log.
- The `AppLogger` class handles file output and rotation automatically; no manual file management is needed.
- Logs are written synchronously to both the file and stderr for real-time visibility.
- Do NOT use `std::cout`, `printf`, or other ad-hoc output mechanisms.

## Release Procedure
- Releases must follow Semantic Versioning (`major.minor.patch`).
- Before creating a release, inspect the commits since the last tag:
  ```bash
  git describe --tags --abbrev=0
  git log --oneline <last-tag>..HEAD
  ```
- Determine the next version number from the actual changes:
  - increment `major` for breaking changes
  - increment `minor` for backward-compatible new features
  - increment `patch` for backward-compatible fixes only
- Update `CHANGELOG.md` first. Add a new section for the new version at the top, with the release date, and make sure the content matches the commits since the last tag.
- Update the project version only with the dedicated script:
  ```bash
  ./scripts/linux/set-version.sh x.y.z
  ```
- Commit the release changes before tagging them.
- Create an annotated Git tag, not a lightweight tag:
  ```bash
  git tag -a "vx.y.z" -m "Version x.y.z"
  ```
- Build the package with:
  ```bash
  ./scripts/linux/package.sh
  ```
- `scripts/linux/package.sh` reads the version from `VERSION`, builds the package, and creates `dist/` automatically if it does not exist.
- After packaging, verify that the expected artifacts for the current version are present in `dist/`.
- Create the GitHub release with:
  - name: `Version x.y.z`
  - tag: `vx.y.z`
  - description: the content of the latest `CHANGELOG.md` entry only
  - binary asset: the package generated in `dist/`
- If `gh` is available, it can be used to create the GitHub release. If not, create the release manually in the GitHub web interface.
- Do not rely on `scripts/linux/release-version.sh` for the official release flow, because it updates tracked files broadly and creates a lightweight tag, while releases must use the explicit changelog update and the annotated tag format above.
