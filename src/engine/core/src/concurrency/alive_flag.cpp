#include "halley/concurrency/alive_flag.h"

using namespace Halley;

AliveFlag::AliveFlag()
	: flag(std::make_shared<bool>(true))
{}

AliveFlag::~AliveFlag()
{
	*flag = false;
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

