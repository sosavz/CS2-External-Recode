# Velora

CS2 External Cheat - Educational Project

## Overview

Velora is an external cheat for Counter-Strike 2 built as a learning project for game hacking fundamentals. It uses memory reading techniques without code injection, making it relatively safer for educational purposes.

## Features

### Combat
- **Legit Aimbot** - Smooth targeting with configurable FOV and smoothing
- **Triggerbot** - Automatic firing when crosshair is on enemy
- **Predictive Aim** - Movement prediction for moving targets
- **Weapon Profiles** - Different settings per weapon category

### ESP (Extra Sensory Perception)
- **Player Box** - Cornered or full box rendering
- **Skeleton** - Bone line rendering
- **Health/Armor Bars** - Visual health indicators
- **Weapon Icons** - Show equipped weapons
- **Info Flags** - Money, armor, kit, scoped status, etc.
- **Grenade Prediction** - Trajectory visualization

### Items & Projectiles
- **Item ESP** - Dropped weapons and equipment
- **Projectile Tracking** - HE, Flash, Smoke, Molotov tracking
- **Fire Zones** - Molotov/incendiary fire visualization

### Miscellaneous
- **Watermark** - FPS display
- **Spectator Warning** - Alert when being watched
- **FPS Limiter** - Performance controls
- **Config System** - Save/load settings

## Architecture

```
source/
├── project/
│   ├── core/                    # Core systems
│   │   ├── config/             # Configuration management
│   │   ├── features/           # Feature implementations
│   │   │   ├── impl/combat/   # Aimbot, triggerbot
│   │   │   └── impl/esp/      # ESP rendering
│   │   ├── menu/              # In-game menu UI
│   │   ├── render/             # Direct3D rendering
│   │   ├── systems/           # Game data collection
│   │   │   └── impl/          # Entity, bones, BVH, etc.
│   │   └── threads/            # Thread management
│   ├── utils/                  # Utilities
│   │   ├── memory/            # Memory reading
│   │   ├── math/              # Vector/matrix math
│   │   └── input/             # Input simulation
│   └── ext/                    # External dependencies
│       └── zdraw/             # 2D rendering library
```

### Key Systems

- **Entity Collection** - Reads player/item data from game memory
- **BVH Ray Tracing** - Line-of-sight checks for visibility
- **Bone System** - Reads bone matrices for skeleton rendering
- **Schema System** - Caches field offsets for memory reads
- **Render System** - Direct3D 11 overlay rendering

## Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ desktop development
- Counter-Strike 2

## Building

1. Clone the repository
2. Open `source/velora.vcxproj` in Visual Studio 2022
3. Select **Release** configuration and **x64** platform
4. Build the solution (Ctrl+Shift+B)
5. Find the compiled binary in `bin/velora_[hash].exe`

## Usage

1. Run the compiled executable
2. Click "Load" to start the loader
3. The loader will inject and wait for CS2 to launch
4. Press **Insert** to toggle the menu
5. Configure features and start playing

## Configuration

Settings are automatically saved to:
```
%APPDATA%/velora/
```

Manual configuration:
- `source/project/core/settings.hpp` - Feature toggles
- `source/project/core/constants.hpp` - Game constants

## Disclaimer

This project is for **educational purposes only**. Using this software in online multiplayer games violates terms of service and can result in a permanent ban. The authors are not responsible for any consequences.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Acknowledgments

- zdraw/zui - Rendering library
- xorstr - String obfuscation
- Various game hacking communities for documentation

## License

See [LICENSE](LICENSE) file for details.
