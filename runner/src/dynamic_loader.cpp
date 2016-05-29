#include "dynamic_loader.h"
#include <halley/runner/halley_main.h>
#include "memory_patcher.h"

using namespace Halley;

DynamicGameLoader::DynamicGameLoader(std::string name)
	: libName(name)
	, lib(name)
{
	load();
}

DynamicGameLoader::~DynamicGameLoader()
{
	unload();
}

std::unique_ptr<IMainLoopable> DynamicGameLoader::createCore(std::vector<std::string> args)
{
	assert(entry != nullptr);
	return entry->createCore(args);
}

std::unique_ptr<Game> DynamicGameLoader::createGame()
{
	throw Exception("Cannot create game from DynamicGameLoader");
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

void DynamicGameLoader::setCore(IMainLoopable& c)
{
	core = &c;
	core->init();
}

#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

void DynamicGameLoader::load()
{
	lib.load(true);
	auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint*(STDCALL*)()>(lib.getFunction("getHalleyEntry"));
	if (!getHalleyEntry) {
		lib.unload();
		throw Exception("getHalleyEntry not found.");
	}

	prevSymbols = std::move(symbols);
	symbols = SymbolLoader::loadSymbols(lib);

	entry = getHalleyEntry();
}

void DynamicGameLoader::unload()
{
	core->onSuspended();

	entry = nullptr;
	lib.unload();
}

void DynamicGameLoader::hotPatch()
{
	MemoryPatchingMappings mappings;
	mappings.generate(prevSymbols, symbols);
	MemoryPatcher::patch(mappings);

	//setStatics();
	core->onReloaded();
}

