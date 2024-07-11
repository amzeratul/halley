#pragma once

namespace Halley {}

#ifdef _MSC_VER
	#include <xmmintrin.h>

	#if defined(DEV_BUILD)
		#define PRAGMA_DEOPTIMIZE __pragma(optimize( "", off ))
		#define PRAGMA_REOPTIMIZE __pragma(optimize( "", on ))
	#else
		#define PRAGMA_DEOPTIMIZE
		#define PRAGMA_REOPTIMIZE
	#endif

	#define NOINLINE __declspec(noinline)
	#define FORCEINLINE __forceinline
#else
	#define PRAGMA_DEOPTIMIZE
	#define PRAGMA_REOPTIMIZE

	#define NOINLINE __attribute__((noinline))
	#ifdef __clang__
		#define FORCEINLINE inline
	#else
		#define FORCEINLINE inline __attribute__((always_inline))
	#endif
#endif
