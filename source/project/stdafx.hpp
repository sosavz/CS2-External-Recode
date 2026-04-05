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
#include <resources/fonts/mochi.hpp>
#include <resources/fonts/pixel7.hpp>
#include <resources/fonts/pretzel.hpp>
#include <resources/fonts/weapons.hpp>

// utilities
#include <utilities/animation/animation.hpp>
#include <utilities/console/console.hpp>
#include <utilities/input/input.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/math/math.hpp>
#include <utilities/modules/modules.hpp>
#include <utilities/offsets/offsets.hpp>
#include <utilities/timing/timing.hpp>
#include <utilities/diagnostics/diagnostics.hpp>
#include <utilities/cstypes.hpp>
#include <utilities/fnv1a.hpp>
#include <utilities/random.hpp>

// core
#include <core/settings.hpp>
#include <core/systems/systems.hpp>
#include <core/render/render.hpp>
#include <core/threads/threads.hpp>
#include <core/features/features.hpp>
#include <core/menu/menu.hpp>
#include <core/config/config.hpp>

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
