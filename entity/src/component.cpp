#include "component.h"

using namespace Halley;

void* Component::operator new(size_t size)
{
	return PoolPool::getPool(size)->alloc();
}

void Component::operator delete(void*) noexcept(false)
{
	throw Exception("Attempting to delete component.");
}
