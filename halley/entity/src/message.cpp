#include "message.h"

using namespace Halley;

/*
void* Message::operator new(size_t size)
{
	return PoolPool::getPool(size)->alloc();
}

void Message::operator delete(void* ptr)
{
	PoolPool::getPool(static_cast<Message*>(ptr)->getSize())->free(ptr);
}
*/
