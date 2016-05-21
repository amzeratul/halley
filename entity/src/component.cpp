#include <halley/data_structures/memory_pool.h>
#include <halley/support/exception.h>
#include "../include/halley/entity/component.h"

using namespace Halley;

void* Component::operator new(size_t size)
{
	return PoolPool::getPool(size)->alloc();
}

void Component::operator delete(void*) noexcept(false)
{
	throw Exception("Attempting to delete component.");
}
