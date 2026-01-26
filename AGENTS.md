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
