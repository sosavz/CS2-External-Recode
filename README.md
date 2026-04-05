# CS2 External Recode

## What This Project Is
CS2 External Recode is a C++ external project structured around modular systems for:
- entity/data collection
- rendering and menu/UI
- feature logic
- shared utilities (math, memory, timing, input, diagnostics)

## Project Layout
- `source/velora.vcxproj` Visual Studio project file
- `source/project/core/` core systems, features, render, menu, threads
- `source/project/utils/` utility modules
- `source/project/ext/` third-party/external dependencies
- `source/project/Res/` embedded resources (fonts/assets)

## Requirements
- Windows
- Visual Studio 2022 with C++ desktop development tools

## Build
1. Open `source/velora.vcxproj` in Visual Studio.
2. Select configuration (`Debug` or `Release`) and target platform (`x64`).
3. Build the project.

## How To Use
1. Build the project to generate the binary.
2. Run the compiled executable from your build output folder.
3. Configure options through the in-project menu/config system.

## Configuration
- Main config logic is under `source/project/core/config/`.
- Feature and system toggles are managed in `source/project/core/settings.hpp` and related feature modules.
