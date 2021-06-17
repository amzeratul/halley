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
		setStatus(Status::DLLNotFound);
		return;
	}
	
	const auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(dll.getFunction("getHalleyEntry"));
	if (!getHalleyEntry) {
		// Not a Halley DLL
		setStatus(Status::InvalidDLL);
		dll.unload();
		return;
	}

	auto* entry = getHalleyEntry();
	if (entry->getApiVersion() != HALLEY_DLL_API_VERSION) {
		// Incompatible version
		setStatus(Status::WrongDLLVersion);
		dll.unload();
		return;
	}

	try {
		entryPoint = entry;
		entryPoint->initSharedStatics(api.core->getStatics());
		game = entryPoint->createGame();

		setStatus(Status::Loaded);
	} catch (const std::exception& e) {
		Logger::logException(e);
		unload();
		setStatus(Status::DLLCrash);
	} catch (...) {
		unload();
		setStatus(Status::DLLCrash);
	}
}

void ProjectDLL::unload()
{
	game.reset();
	entryPoint = nullptr;
	dll.unload();

	setStatus(Status::Unloaded);
}

bool ProjectDLL::isLoaded() const
{
	return status == Status::Loaded;
}

void ProjectDLL::notifyReload()
{
	dll.notifyReload();
}

void ProjectDLL::reloadIfChanged()
{
	dll.reloadIfChanged();
}

void ProjectDLL::addReloadListener(IProjectDLLListener& listener)
{
	reloadListeners.insert(&listener);
}

void ProjectDLL::removeReloadListener(IProjectDLLListener& listener)
{
	reloadListeners.erase(&listener);
}

Game& ProjectDLL::getGame() const
{
	Expects(game != nullptr);
	return *game;
}

ProjectDLL::Status ProjectDLL::getStatus() const
{
	return status;
}

void ProjectDLL::setStatus(Status s)
{
	if (status != s) {
		Logger::logInfo("DLL status change: " + toString(status) + " -> " + toString(s));
		
		status = s;
		for (const auto& listener: reloadListeners) {
			listener->onProjectDLLStatusChange(s);
		}
	}
}
