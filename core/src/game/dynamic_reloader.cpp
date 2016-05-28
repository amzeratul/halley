#include "game/dynamic_reloader.h"

using namespace Halley;

DynamicGameReloader::DynamicGameReloader(String name)
	: dllName(name)
{
}

std::unique_ptr<Game> DynamicGameReloader::createGame()
{
	return std::unique_ptr<Game>();
}

bool DynamicGameReloader::needsToReload() const
{
	return false;
}

void DynamicGameReloader::reload()
{
	
}
