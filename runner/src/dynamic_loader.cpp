#include "dynamic_loader.h"
#include <halley/runner/halley_main.h>
#include "memory_patcher.h"

using namespace Halley;

DynamicGameLoader::DynamicGameLoader(String name)
	: libName(name)
	, lib(name)
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
	Stopwatch timer;
	
	unload();
	load();
	hotPatch();

	timer.pause();
	
	std::cout << "Done in " << timer.elapsedSeconds() << " seconds.\n" << ConsoleColor() << std::endl;
}

void DynamicGameLoader::setCore(Core& c)
{
	core = &c;
	setStatics();
}

#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

void DynamicGameLoader::load()
{
	lib.load(true);
	auto createHalleyEntry = reinterpret_cast<IHalleyEntryPoint*(STDCALL*)()>(lib.getFunction("createHalleyEntry"));
	if (!createHalleyEntry) {
		lib.unload();
		throw Exception("createHalleyEntry not found.");
	}

	prevSymbols = std::move(symbols);
	symbols = SymbolLoader::loadSymbols(lib);

	entry = createHalleyEntry();
}

void DynamicGameLoader::unload()
{
	if (entry) {
		auto deleteHalleyEntry = reinterpret_cast<void(STDCALL*)(IHalleyEntryPoint*)>(lib.getFunction("deleteHalleyEntry"));
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

	MemoryPatchingMappings mappings;
	mappings.generate(prevSymbols, symbols);
	MemoryPatcher::patch(mappings);

	core->onReloaded();
}

void DynamicGameLoader::setStatics()
{
	auto setupStatics = reinterpret_cast<void(STDCALL*)(HalleyStatics*)>(lib.getFunction("setupStatics"));
	if (!setupStatics) {
		throw Exception("setupStatics not found.");
	}
	setupStatics(&core->getStatics());
}
