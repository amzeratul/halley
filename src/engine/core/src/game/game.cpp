#include "halley/game/game.h"

#include "halley/editor_extensions/asset_preview_generator.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/scripting/script_node_type.h"
#include "halley/ui/ui_factory.h"
using namespace Halley;

Game::~Game() = default;

void Game::init(const Environment&, const Vector<String>&)
{}

ResourceOptions Game::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{
	return ResourceOptions(false, false, isDevMode());
}

void Game::update(Time t)
{
	if (i18n) {
		i18n->update();
	}
}

String Game::getLogFileName() const
{
    return "log.txt";
}

bool Game::shouldCreateSeparateConsole() const
{
	return isDevMode();
}

Game::ConsoleInfo Game::getConsoleInfo() const
{
	return ConsoleInfo{ getName() + " [Console]", {}, Vector2f(0.5f, 0.5f) };
}

void Game::endGame()
{}

std::unique_ptr<Stage> Game::makeStage(StageID)
{
	return {};
}

std::unique_ptr<BaseFrameData> Game::makeFrameData()
{
	return std::make_unique<DefaultFrameData>();
}

double Game::getTargetFPS() const
{
	return 0.0;
}

double Game::getTargetBackgroundFPS() const
{
	return getTargetFPS();
}

double Game::getFixedUpdateFPS() const
{
	return 60.0;
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
	return {};
}

std::unique_ptr<IEditorCustomTools> Game::createEditorCustomToolsInterface()
{
	return {};
}

std::unique_ptr<AssetPreviewGenerator> Game::createAssetPreviewGenerator(const HalleyAPI& api, Resources& resources, IGameEditorData* gameEditorData)
{
	return std::make_unique<AssetPreviewGenerator>(*this, api, resources);
}

std::unique_ptr<UIFactory> Game::createUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
{
	auto factory = std::make_unique<UIFactory>(api, resources, i18n);
	const auto colourScheme = getDefaultColourScheme();
	if (!colourScheme.isEmpty()) {
		factory->setColourScheme(colourScheme);
	}
	factory->loadStyleSheetsFromResources();
	return factory;
}

std::unique_ptr<ScriptNodeTypeCollection> Game::createScriptNodeTypeCollection()
{
	return std::make_unique<ScriptNodeTypeCollection>();
}

Vector<std::unique_ptr<IComponentEditorFieldFactory>> Game::createCustomEditorFieldFactories(Resources& gameResources, IGameEditorData* gameEditorData)
{
	return {};
}

Vector<std::unique_ptr<IComponentEditorFieldFactory>> Game::createCustomScriptEditorFieldFactories(const Scene& scene, Resources& resources, IGameEditorData* gameEditorData)
{
	return {};
}

std::unique_ptr<IGameEditorData> Game::createGameEditorData(const HalleyAPI& api, Resources& resources)
{
	return {};
}

Vector<ConfigBreadCrumb> Game::createConfigBreadCrumbs()
{
	return {};
}

String Game::getLocalisationFileCategory(const String& assetName)
{
	return "unknown";
}

bool Game::canCollectVideoPerformance()
{
	return true;
}

const ResourceUnloaderRules& Game::getResourceUnloaderRules() const
{
	return resourceUnloaderRules;
}

ResourceUnloaderRules& Game::getResourceUnloaderRules()
{
	return resourceUnloaderRules;
}

String Game::getDefaultColourScheme()
{
	return "";
}

void Game::attachToEditorDebugConsole(UIDebugConsoleCommands& commands, Resources& gameResources, IProject& project)
{
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

const I18N& Game::getI18N() const
{
	if (!i18n) {
		throw Exception("Game I18N has not been initialised", HalleyExceptions::Core);
	}
	return *i18n;
}

const I18N* Game::tryGetI18N() const
{
	return i18n.get();
}

I18N& Game::getI18N()
{
	if (!i18n) {
		throw Exception("Game I18N has not been initialised", HalleyExceptions::Core);
	}
	return *i18n;
}

void Game::setI18N(std::unique_ptr<I18N> i18n)
{
	this->i18n = std::move(i18n);
}

std::optional<int> Game::getCurrentDisplay() const
{
	auto& video = *getAPI().video;
	if (video.hasWindow()) {
		const auto windowRect = video.getWindow().getWindowRect();
		const auto windowCentre = windowRect.getCenter();

		auto& system = *getAPI().system;
		int nDisplays = system.getNumDisplays();
		for (int i = 0; i < nDisplays; ++i) {
			if (system.getDisplayRect(i).contains(windowCentre)) {
				return i;
			}
		}
		return video.getWindow().getDefinition().getScreen();
	}
	return {};
}

UIDebugConsoleCommands& Game::initBaseCommands()
{
	baseCommands.addCommand("displayRes", [=] (Vector<String> args) -> String {
		const auto idx = args[0].toInteger();
		const auto res = Vector2i(args[1].toInteger(), args[2].toInteger());
		const auto refresh = args[3].toInteger();
		if (auto result = api->system->setDisplayRenderTarget(1, res, refresh)) {
			return "Setting display #" + toString(idx) + " to " + toString(result->first.x) + "x" + toString(result->first.y) + " @ " + toString(result->second) + " Hz";
		} else {
			return "Couldn't set resolution for display #" + toString(idx);
		}
	}, { { { "idx", "int" }, { "resX", "int" }, { "resY", "int" }, { "refreshRate", "int" } } });

	return baseCommands;
}

UIDebugConsoleCommands& Game::initBatchCommands(IUIDebugConsoleController& controller)
{
	if (isDevMode()) {
		if (const auto& batches = getResources().tryGet<ConfigFile>("dev/command_batches")) {
			batchCommands.addCommandBatches(batches->getRoot(), controller);
		}

		const auto userPath = getAPI().core->getEnvironment().getDataPath() / "user_command_batches.yaml";
		if (Path::exists(userPath)) {
			Logger::logDev("Reading user batch commands from " + userPath.getNativeString(false));
			const auto& data = Path::readFileString(userPath);
			if (!data.isEmpty()) {
				const auto& config = YAMLConvert::parseConfig(data);
				batchCommands.addCommandBatches(config, controller);
			}
		}
	}

	return batchCommands;
}

size_t Game::getMaxThreads(const ComputerData& computerData) const
{
	size_t n = std::thread::hardware_concurrency();

	if constexpr (isPCPlatform()) {
		// Prevent broken Intel CPUs from melting
		if (computerData.cpuName.contains("i9-13900K") || computerData.cpuName.contains("i9-14900K")) {
			const auto prev = n;
			n = std::min<size_t>(n, 8);
			Logger::logInfo("Throttling from " + toString(prev) + " to " + toString(n) + " threads due to i9-13900K/i9-14900K detected.");
		}
	}

	Logger::logInfo("Using " + toString(n) + " hardware threads.");
	return n;
}

bool Game::shouldProcessEventsOnFixedUpdate() const
{
	return false;
}
