#include "halley/concurrency/shared_recursive_mutex.h"
#include <thread>

using namespace Halley;

void SharedRecursiveMutex::lock()
{
	auto thisId = std::this_thread::get_id();
	if (owner == thisId) {
		count++;
	} else {
		mutex.lock();
		owner = thisId;
		count = 1;
	}
}

bool SharedRecursiveMutex::try_lock()
{
	auto thisId = std::this_thread::get_id();
	if (owner == thisId) {
		count++;
		return true;
	} else {
		const bool result = mutex.try_lock();
		if (result) {
			owner = thisId;
			count = 1;
		}
		return result;
	}
}

void SharedRecursiveMutex::unlock()
{
	if (count > 1) {
		--count;
	} else {
		owner = std::thread::id();
		count = 0;
		mutex.unlock();
	}
}

void SharedRecursiveMutex::lock_shared()
{
	auto thisId = std::this_thread::get_id();
	if (owner == thisId) {
		count++;
	} else {
		mutex.lock_shared();
		owner = thisId;
		count = 1;
	}
}

bool SharedRecursiveMutex::try_lock_shared()
{
	auto thisId = std::this_thread::get_id();
	if (owner == thisId) {
		count++;
		return true;
	} else {
		const bool result = mutex.try_lock_shared();
		if (result) {
			owner = thisId;
			count = 1;
		}
		return result;
	}
}

void SharedRecursiveMutex::unlock_shared()
{
	if (count > 1) {
		--count;
	} else {
		owner = std::thread::id();
		count = 0;
		mutex.unlock_shared();
	}
}
