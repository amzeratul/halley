#include <halley/data_structures/memory_pool.h>
#include <halley/support/exception.h>
#include "component.h"
#include <iostream>
#include <halley/support/console.h>

using namespace Halley;

void* Component::operator new(size_t size)
{
	return PoolPool::getPool(size)->alloc();
}

void Component::operator delete(void*)
{
	std::cout << ConsoleColor(Console::RED) << "Attempting to delete component!" << ConsoleColor() << std::endl;
}
