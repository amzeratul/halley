#include "halley/tools/runner/dynamic_loader.h"
#include <halley/core/game/halley_main.h>
#include <halley/core/entry/entry_point.h>
#include <gsl/gsl_assert>
#include "halley/support/console.h"
#include "halley/tools/runner/memory_patcher.h"

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

std::unique_ptr<Core> DynamicGameLoader::createCore(std::vector<std::string> args)
{
	Expects(entry != nullptr);
	return entry->createCore(args);
}

std::unique_ptr<Game> DynamicGameLoader::createGame()
{
	return entry->createGame();
}

bool DynamicGameLoader::needsToReload() const
{
	return lib.hasChanged();
}

void DynamicGameLoader::reload()
{
	std::cout << ConsoleColour(Console::BLUE) << "\n**RELOADING GAME**" << std::endl;
	Stopwatch timer;
	
	core->onSuspended();
	unload();
	load();
	hotPatch();
	core->onReloaded();

	timer.pause();
	
	std::cout << "Done in " << timer.elapsedSeconds() << " seconds.\n" << ConsoleColour() << std::endl;
}

void DynamicGameLoader::setCore(IMainLoopable& c)
{
	core = &c;
}

void DynamicGameLoader::load()
{
	lib.load(true);
	
	auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint*(HALLEY_STDCALL*)()>(lib.getFunction("getHalleyEntry"));
	if (!getHalleyEntry) {
		lib.unload();
		throw Exception("getHalleyEntry not found.", HalleyExceptions::Core);
	}

	entry = getHalleyEntry();
	if (entry->getApiVersion() != HALLEY_DLL_API_VERSION) {
		lib.unload();
		throw Exception("Halley API mismatch.", HalleyExceptions::Core);
	}
	
	prevSymbols = std::move(symbols);
	symbols = SymbolLoader::loadSymbols(lib);
}

void DynamicGameLoader::unload()
{
	entry = nullptr;
	lib.unload();
}

void DynamicGameLoader::hotPatch()
{
	MemoryPatchingMappings mappings;
	mappings.generate(prevSymbols, symbols);
	MemoryPatcher::patch(mappings);
}

