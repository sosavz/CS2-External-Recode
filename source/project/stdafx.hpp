#pragma once

// macros
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// windows
#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <windowsx.h>

// standard
#include <array>
#include <vector>
#include <string>
#include <numbers>
#include <algorithm>
#include <format>
#include <filesystem>
#include <unordered_map>
#include <shared_mutex>
#include <unordered_set>
#include <chrono>

// external
#include <ext/zdraw/zui/zui.hpp>
#include <ext/poly2d.hpp>
#include <ext/xorstr.hpp>

// resources
#include <Res/fonts/mochi.hpp>
#include <Res/fonts/pixel7.hpp>
#include <Res/fonts/pretzel.hpp>
#include <Res/fonts/weapons.hpp>

// utilities
#include <utils/animation/animation.hpp>
#include <utils/console/console.hpp>
#include <utils/input/input.hpp>
#include <utils/memory/memory.hpp>
#include <utils/math/math.hpp>
#include <utils/modules/modules.hpp>
#include <utils/offsets/offsets.hpp>
#include <utils/timing/timing.hpp>
#include <utils/diagnostics/diagnostics.hpp>
#include <utils/cstypes.hpp>
#include <utils/fnv1a.hpp>
#include <utils/random.hpp>

// core
#include <core/constants.hpp>
#include <core/settings.hpp>
#include <core/systems/systems.hpp>
#include <core/render/render.hpp>
#include <core/threads/threads.hpp>
#include <core/features/features.hpp>
#include <core/menu/menu.hpp>
#include <core/config/config.hpp>
#include <core/perf/perf.hpp>

namespace g {

	inline console console{};
	inline input input{};
	inline memory memory{};
	inline modules modules{};
	inline offsets offsets{};

	inline render render{};
	inline menu menu{};
	inline config_system config{};

} // namespace g
