#pragma once

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

namespace Halley {
	enum class GamePlatform {
		Unknown,
		Windows,
		MacOS,
		Linux,
		Switch,
		XboxOne,
		PS4,
		UWP,
		Android,
		iOS,
        Emscripten
	};

	template <>
	struct EnumNames<GamePlatform> {
		constexpr std::array<const char*, 11> operator()() const {
			return {{
				"unknown",
				"windows",
				"macos",
				"linux",
				"switch",
				"xboxone",
				"ps4",
				"uwp",
				"android",
				"ios",
                "emscripten"
			}};
		}
	};

    constexpr inline GamePlatform getPlatform()
    {
    #if defined(__NX_TOOLCHAIN_MAJOR__)
        return GamePlatform::Switch;
    #elif defined(__ORBIS__)
        return GamePlatform::PS4;
    #elif defined(_XBOX_ONE)
        return GamePlatform::XboxOne;
    #elif defined(WINDOWS_STORE)
        return GamePlatform::UWP;
    #elif defined(_WIN32)
        return GamePlatform::Windows;
    #elif defined(__ANDROID__)
        return GamePlatform::Android;
    #elif defined(__EMSCRIPTEN__)
        return GamePlatform::Emscripten;
    #elif defined(__APPLE__)
        #if TARGET_OS_IPHONE
            return GamePlatform::iOS;
        #else
            return GamePlatform::MacOS;
        #endif
    #elif defined(__linux)
        return GamePlatform::Linux;
    #else
        return GamePlatform::Unknown;
    #endif
    }
}
