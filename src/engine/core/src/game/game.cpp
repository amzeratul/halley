#include "halley/core/game/game.h"
using namespace Halley;

Game::~Game() = default;

void Game::init(const Environment&, const Vector<String>&)
{}

void Game::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{}

bool Game::shouldCreateSeparateConsole() const
{
	return isDevMode();
}

void Game::endGame()
{}

std::unique_ptr<Stage> Game::makeStage(StageID)
{
	return std::unique_ptr<Stage>();
}

int Game::getTargetFPS() const
{
	return 60;
}

String Game::getDevConAddress() const
{
	return "";
}

int Game::getDevConPort() const
{
	return 12500;
}

std::shared_ptr<GameConsole> Game::getGameConsole() const
{
	return {};
}

void Game::onUncaughtException(const Exception& exception, TimeLine timeLine)
{
	throw exception;
}

std::unique_ptr<ISceneEditor> Game::createSceneEditorInterface()
{
	throw Exception("Method not implemented: Game::createSceneEditorInterface", HalleyExceptions::Core);
}

std::unique_ptr<IEditorCustomTools> Game::createEditorCustomToolsInterface()
{
	throw Exception("Method not implemented: Game::createEditorCustomToolsInterface", HalleyExceptions::Core);
}

const HalleyAPI& Game::getAPI() const
{
	if (!api) {
		throw Exception("HalleyAPI is only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *api;
}

Resources& Game::getResources() const
{
	if (!resources) {
		throw Exception("Resources are only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *resources;
}
