#include "halley/data_structures/temp_allocator.h"

#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
#include "halley/utils/utils.h"

using namespace Halley;

TempMemoryPool::TempMemoryPool(size_t capacity, bool allowPaging)
	: capacity(capacity)
	, allowPaging(allowPaging)
{
	allocBuffer();
}

TempMemoryPool::~TempMemoryPool()
{
	reset();
	freeBuffer();
}

char* TempMemoryPool::allocate(size_t n, size_t alignment)
{
	auto p = alignUp(pos, alignment);
	if (p + n < capacity) {
		char* result = data + p;
		assert(reinterpret_cast<size_t>(result) % alignment == 0);
		pos = p + n;
		allocated += n;
		return result;
	}

	// Pool is full!
	if (allowPaging && n < capacity) {
		if (!nextPage) {
			nextPage = std::make_unique<TempMemoryPool>(capacity, allowPaging);
		}
		return nextPage->allocate(n, alignment);
	}

	auto e = Halley::Exception("Out of space on temp memory pool, requested = " + String::prettySize(n) + ", capacity = " + String::prettySize(capacity) + ", free = " + String::prettySize(capacity - pos), HalleyExceptions::DataStructures);
	Logger::logException(e);

	throw std::bad_alloc();
}

void TempMemoryPool::deallocate(void* ptr, size_t n)
{
	if (ptr >= data && ptr < data + capacity) {
		if (allocated >= n) {
			allocated -= n;
		} else {
			Logger::logError("TempMemoryPool received invalid deallocate request.");
		}
	} else {
		if (nextPage) {
			nextPage->deallocate(ptr, n);
		} else {
			Logger::logError("TempMemoryPool received deallocate request for data that doesn't belong to it.");
		}
	}
}

void TempMemoryPool::reset()
{
	if (allocated > 0) {
		Logger::logError("TempMemoryPool deallocating data while some of it is still in use!");
	}

	pos = 0;
	allocated = 0;

	if (nextPage) {
		nextPage->reset();
	}
}

void TempMemoryPool::resize(size_t size)
{
	reset();
	freeBuffer();
	nextPage = {};
	capacity = size;
	allocBuffer();
}

void TempMemoryPool::allocBuffer()
{
	data = new char[capacity];
	pos = 0;
}

void TempMemoryPool::freeBuffer()
{
	delete[] data;
	data = nullptr;
}
