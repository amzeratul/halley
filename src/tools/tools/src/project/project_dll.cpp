#include "halley/tools/project/project_dll.h"



#include "halley/core/api/halley_api.h"
#include "halley/core/entry/entry_point.h"
#include "halley/file/path.h"
#include "halley/support/logger.h"

using namespace Halley;

ProjectDLL::ProjectDLL(const Path& path, const HalleyAPI& api)
	: dll(path.string(), false)
	, api(api)
	, path(path)
{
}

ProjectDLL::~ProjectDLL()
{}

void ProjectDLL::load()
{
	if (status == Status::Loaded) {
		unload();
	}
	
	dll.clearTempDirectory();
	if (!dll.load(true)) {
		// Could not load DLL
		status = Status::DLLNotFound;
		return;
	}
	
	const auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(dll.getFunction("getHalleyEntry"));
	if (!getHalleyEntry) {
		// Not a Halley DLL
		status = Status::InvalidDLL;
		dll.unload();
		return;
	}

	auto* entry = getHalleyEntry();
	if (entry->getApiVersion() != HALLEY_DLL_API_VERSION) {
		// Incompatible version
		status = Status::WrongDLLVersion;
		dll.unload();
		return;
	}

	try {
		entryPoint = entry;
		entryPoint->initSharedStatics(api.core->getStatics());
		game = entryPoint->createGame();

		status = Status::Loaded;
		Logger::logInfo("Loaded " + path.string());
	} catch (const std::exception& e) {
		Logger::logException(e);
		unload();
		status = Status::DLLCrash;
	} catch (...) {
		Logger::logError("Failed to load DLL " + path.string());
		unload();
		status = Status::DLLCrash;
	}
}

void ProjectDLL::unload()
{
	status = Status::Unloaded;
	game.reset();
	entryPoint = nullptr;
	dll.unload();
}

bool ProjectDLL::isLoaded() const
{
	return entryPoint != nullptr;
}

void ProjectDLL::notifyReload()
{
	dll.notifyReload();
}

void ProjectDLL::reloadIfChanged()
{
	dll.reloadIfChanged();
}

void ProjectDLL::addReloadListener(IDynamicLibraryListener& listener)
{
	dll.addReloadListener(listener);
}

void ProjectDLL::removeReloadListener(IDynamicLibraryListener& listener)
{
	dll.removeReloadListener(listener);
}

Game& ProjectDLL::getGame() const
{
	return *game;
}

ProjectDLL::Status ProjectDLL::getStatus() const
{
	return status;
}
