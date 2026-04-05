# CS2 External Recode

## Overview
This repository contains a C++ Visual Studio project for external experimentation around Counter-Strike 2 data access and rendering pipelines.

The codebase is organized as an engine-style project with modular systems for:
- configuration and settings
- rendering and UI
- math, memory, and module utilities
- feature and system orchestration

## Project Structure
- `source/project/core/` main systems, features, rendering, threading, and menu logic
- `source/project/utils/` low-level helpers (math, memory, diagnostics, timing, input)
- `source/project/ext/` third-party and external support code
- `source/project/Res/` embedded resources (fonts and related assets)

## Build
1. Open `source/velora.vcxproj` in Visual Studio (Windows).
2. Select your desired `Debug` or `Release` configuration.
3. Build the solution.

## Privacy and Repository Hygiene
- Local build outputs and generated IDE artifacts are ignored via `.gitignore`.
- No API keys, tokens, or credential files should be committed.
- If you accidentally commit sensitive data, rotate secrets first, then purge git history.

## Disclaimer
Use this project responsibly and in compliance with all applicable game, platform, and legal policies.
