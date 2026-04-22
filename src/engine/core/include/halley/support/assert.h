#pragma once

#include <string_view>


namespace Halley {
	[[noreturn]] void halleyAssert(std::string_view str, std::string_view filename, int line);
}


#if defined(__clang__) || defined(__GNUG__)
	#define HalleyLikely(expr) (__builtin_expect(!!(expr), 1))
	#define HalleyUnlikely(expr) (__builtin_expect(!!(expr), 0))
#else
	#define HalleyLikely(expr) (expr)
	#define HalleyUnlikely(expr) (expr)
#endif


// This is always available
#define HalleyAssertRelease(expression) do { \
	if (!(expression)) [[unlikely]] \
		Halley::halleyAssert(#expression, __FILE__, __LINE__); \
} while (false)

// Dev builds
#if defined(DEV_BUILD) || defined(_DEBUG)
	#define HalleyAssertDev(expression) HalleyAssertRelease(expression)
#else
	#define HalleyAssertDev(expression) (static_cast<void>(0))
#endif

// Debug builds
#ifdef _DEBUG
	#define HalleyAssertDebug(expression) HalleyAssertRelease(expression)
#else
	#define HalleyAssertDebug(expression) (static_cast<void>(0))
#endif


// Based on the code from imgui
// https://github.com/ocornut/imgui/blob/v1.86/imgui_internal.h#L257
#if defined (_MSC_VER)
	#define HalleyDebugBreak() __debugbreak()
#elif defined(__clang__)
	#define HalleyDebugBreak() __builtin_debugtrap()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	#define HalleyDebugBreak() __asm__ volatile("int $0x03")
#elif defined(__GNUC__) && defined(__thumb__)
	#define HalleyDebugBreak() __asm__ volatile(".inst 0xde01")
#elif defined(__GNUC__) && defined(__arm__) && !defined(__thumb__)
	#define HalleyDebugBreak() __asm__ volatile(".inst 0xe7f001f0")
#else
	#define HalleyDebugBreak() HalleyAssertDev(false)
#endif

#define HalleyDebugBreakIf(expr) do { \
	if (expr) [[unlikely]] \
		HalleyDebugBreak(); \
} while(false)
