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



#define HalleyAssertRelease(expression) ( \
	(HalleyLikely(expression) ? static_cast<void>(0) : Halley::halleyAssert(#expression, __FILE__, __LINE__)) \
)


#if defined(DEV_BUILD) || defined(_DEBUG)
	#define HalleyAssertDev(expression) ( \
		(HalleyLikely(expression) ? static_cast<void>(0) : Halley::halleyAssert(#expression, __FILE__, __LINE__)) \
	)
#else
	#define HalleyAssertDev(expression) (static_cast<void>(0))
#endif



#ifdef _DEBUG
	#define HalleyAssertDebug(expression) ( \
		(HalleyLikely(expression) ? static_cast<void>(0) : Halley::halleyAssert(#expression, __FILE__, __LINE__)) \
	)
#else
	#define HalleyAssertDebug(expression) (static_cast<void>(0))
#endif
