#include "halley/concurrency/alive_flag.h"

using namespace Halley;

AliveFlag::AliveFlag()
	: flag(std::make_shared<bool>(true))
{}

AliveFlag::~AliveFlag()
{
	if (flag) {
		*flag = false;
	}
}

AliveFlag::AliveFlag(const AliveFlag& other)
	: flag(std::make_shared<bool>(true))
{
}

AliveFlag& AliveFlag::operator=(const AliveFlag& other)
{
	if (this != &other) {
		flag = std::make_shared<bool>(true);
	}
	return *this;
}

AliveFlag::operator bool() const
{
	return *flag;
}

NonOwningAliveFlag::NonOwningAliveFlag(const AliveFlag& flag)
	: flag(flag.flag)
{
}

NonOwningAliveFlag::operator bool() const
{
	return *flag;
}

