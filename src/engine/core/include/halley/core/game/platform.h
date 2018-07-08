#pragma once

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

namespace Halley {
	enum class Platform {
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
	struct EnumNames<Platform> {
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

    constexpr inline Platform getPlatform()
    {
    #if defined(__NX_TOOLCHAIN_MAJOR__)
        return Platform::Switch;
    #elif defined(__ORBIS__)
        return Platform::PS4;
    #elif defined(_XBOX_ONE)
        return Platform::XboxOne;
    #elif defined(WINDOWS_STORE)
        return Platform::UWP;
    #elif defined(_WIN32)
        return Platform::Windows;
    #elif defined(__ANDROID__)
        return Platform::Android;
    #elif defined(__EMSCRIPTEN__)
        return Platform::Emscripten;
    #elif defined(__APPLE__)
        #if defined(TARGET_OS_IPHONE)
            return Platform::iOS;
        #else
            return Platform::Mac;
        #endif
    #elif defined(__linux)
        return Platform::Linux;
    #else
        return Platform::Unknown;
    #endif
    }
}
