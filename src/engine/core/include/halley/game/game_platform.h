#pragma once

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#include "halley/text/enum_names.h"

namespace Halley {
	enum class GamePlatform {
		Unknown,
		Windows,
		MacOS,
		Linux,
		Switch,
		XboxOne,
		PS4,
		WindowsGDK,
		Android,
		iOS,
        Emscripten,
		FreeBSD,
		XboxSeries,
		PS5,
        FuturePlatform1,
        FuturePlatform2,
        FuturePlatform3,
        FuturePlatform4,
        FuturePlatform5,
	};

	template <>
	struct EnumNames<GamePlatform> {
		constexpr std::array<const char*, 19> operator()() const {
			return {{
				"unknown",
				"windows",
				"macos",
				"linux",
				"switch",
				"xboxone",
				"ps4",
				"windowsgdk",
				"android",
				"ios",
                "emscripten",
				"freebsd",
				"xboxseries",
				"ps5",
                "futurePlatform1",
                "futurePlatform2",
                "futurePlatform3",
                "futurePlatform4",
                "futurePlatform5"
			}};
		}
	};

    constexpr inline GamePlatform getPlatform()
    {
	#if defined(__FUTURE_PLATFORM_1__)
        return GamePlatform::FuturePlatform1;
	#elif defined(__FUTURE_PLATFORM_2__)
        return GamePlatform::FuturePlatform2;
	#elif defined(__FUTURE_PLATFORM_3__)
        return GamePlatform::FuturePlatform3;
	#elif defined(__FUTURE_PLATFORM_4__)
        return GamePlatform::FuturePlatform4;
	#elif defined(__FUTURE_PLATFORM_5__)
        return GamePlatform::FuturePlatform5;
	#elif defined(__NX_TOOLCHAIN_MAJOR__)
        return GamePlatform::Switch;
    #elif defined(__ORBIS__)
        return GamePlatform::PS4;
    #elif defined(_GAMING_XBOX_XBOXONE)
        return GamePlatform::XboxOne;
	#elif defined(_GAMING_XBOX_SCARLETT)
    		return GamePlatform::XboxSeries;
	#elif defined(_GAMING_DESKTOP)
        return GamePlatform::WindowsGDK;
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
    #elif defined(__FreeBSD__)
        return GamePlatform::FreeBSD;
    #else
        return GamePlatform::Unknown;
    #endif
    }

    constexpr inline bool isPCPlatform(GamePlatform platform = getPlatform())
    {
        return platform == GamePlatform::Windows
            || platform == GamePlatform::WindowsGDK
    		|| platform == GamePlatform::Linux
    		|| platform == GamePlatform::MacOS
    		|| platform == GamePlatform::FreeBSD
    		|| platform == GamePlatform::Emscripten;
    }

    constexpr inline bool isMobilePlatform(GamePlatform platform = getPlatform())
    {
        return platform == GamePlatform::Android
    		|| platform == GamePlatform::iOS;
    }

    constexpr inline bool isConsolePlatform(GamePlatform platform = getPlatform())
    {
        return platform == GamePlatform::Switch
            || platform == GamePlatform::PS4
            || platform == GamePlatform::PS5
            || platform == GamePlatform::XboxOne
            || platform == GamePlatform::XboxSeries
            || platform == GamePlatform::FuturePlatform1
            || platform == GamePlatform::FuturePlatform2
            || platform == GamePlatform::FuturePlatform3
            || platform == GamePlatform::FuturePlatform4
            || platform == GamePlatform::FuturePlatform5;
    }
}
