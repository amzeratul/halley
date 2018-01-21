#if HAS_EASTL

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <stdlib.h>
#endif

void* operator new[](size_t size, const char* /*name*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
	return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t /*alignmentOffset*/, const char* /*name*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
#ifdef _MSC_VER
	return _aligned_malloc(size, alignment);
#else
	void* result;
	posix_memalign(&result, alignment, size);
	return result;
#endif
}

#endif
