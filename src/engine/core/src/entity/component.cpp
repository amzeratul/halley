#include <halley/data_structures/memory_pool.h>
#include <halley/support/exception.h>
#include "halley/entity/component.h"
#include <iostream>
#include <halley/support/console.h>

using namespace Halley;

void* Component::operator new(size_t size)
{
	return PoolPool::getPool(size)->alloc();
}

void Component::operator delete(void*)
{
	std::cout << ConsoleColour(Console::RED) << "Attempting to delete component!" << ConsoleColour() << std::endl;
}
