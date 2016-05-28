#include "dynamic_loader.h"
#include <halley/core/game/game.h>

using namespace Halley;

DynamicGameLoader::DynamicGameLoader(String name)
	: dllName(name)
{
}

std::unique_ptr<Game> DynamicGameLoader::createGame()
{
	throw Exception("TODO: DynamicGameLoader::createGame()");
}

bool DynamicGameLoader::needsToReload() const
{
	return false;
}

void DynamicGameLoader::reload()
{
	
}
