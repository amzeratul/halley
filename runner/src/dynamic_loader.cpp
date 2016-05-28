#include "dynamic_loader.h"
#include <halley/core/game/game.h>
#include <halley/runner/halley_main.h>

using namespace Halley;

DynamicGameLoader::DynamicGameLoader(String name)
	: lib(name)
{
	load();
}

DynamicGameLoader::~DynamicGameLoader()
{
	unload();
}

std::unique_ptr<Game> DynamicGameLoader::createGame()
{
	return entry->makeGame();
}

bool DynamicGameLoader::needsToReload() const
{
	return lib.hasChanged();
}

void DynamicGameLoader::reload()
{
	std::cout << ConsoleColor(Console::BLUE) << "\n**RELOADING GAME**" << std::endl;
	std::cout << "Unloading..." << std::endl;
	unload();
	std::cout << "Loading..." << std::endl;
	load();
	std::cout << "Hot-patching..." << std::endl;
	hotPatch();
	std::cout << "Done!\n" << ConsoleColor() << std::endl;
}

void DynamicGameLoader::setCore(Core& c)
{
	core = &c;
	setStatics();
}

void DynamicGameLoader::load()
{
	lib.load(true);
	auto createHalleyEntry = reinterpret_cast<IHalleyEntryPoint*(__stdcall*)()>(lib.getFunction("createHalleyEntry"));
	if (!createHalleyEntry) {
		lib.unload();
		throw Exception("createHalleyEntry not found.");
	}
	entry = createHalleyEntry();
}

void DynamicGameLoader::unload()
{
	if (entry) {
		auto deleteHalleyEntry = reinterpret_cast<void(__stdcall*)(IHalleyEntryPoint*)>(lib.getFunction("deleteHalleyEntry"));
		if (!deleteHalleyEntry) {
			throw Exception("deleteHalleyEntry not found.");
		}
		deleteHalleyEntry(entry);
		entry = nullptr;
	}
	lib.unload();
}

void DynamicGameLoader::hotPatch()
{
	setStatics();
	core->onReloaded();
}

void DynamicGameLoader::setStatics()
{
	auto setupStatics = reinterpret_cast<void(__stdcall*)(HalleyStatics*)>(lib.getFunction("setupStatics"));
	if (!setupStatics) {
		throw Exception("setupStatics not found.");
	}
	setupStatics(&core->getStatics());
}
