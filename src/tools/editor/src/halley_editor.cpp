#include "halley_editor.h"
#include "editor_root_stage.h"
#include "halley/tools/project/project.h"
#include "preferences.h"
#include "halley/game/environment.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project_loader.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry, std::optional<String> cryptKey);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry, bool allowXInput);
void initSDL3SystemPlugin(IPluginRegistry& registry, std::optional<String> cryptKey);
void initSDL3AudioPlugin(IPluginRegistry& registry);
void initSDL3InputPlugin(IPluginRegistry& registry);
void initAsioPlugin(IPluginRegistry &registry);
void initDX11Plugin(IPluginRegistry &registry);
void initDX12Plugin(IPluginRegistry& registry);
void initMetalPlugin(IPluginRegistry &registry);
void initHTTPLibPlugin(IPluginRegistry& registry);

HalleyEditor::HalleyEditor()
{
}

HalleyEditor::~HalleyEditor()
{
}

int HalleyEditor::initPlugins(IPluginRegistry &registry)
{
#if defined(WITH_SDL3)
	initSDL3SystemPlugin(registry, std::nullopt);
	initSDL3AudioPlugin(registry);
	initSDL3InputPlugin(registry);
#else
	initSDLSystemPlugin(registry, {});
	initSDLAudioPlugin(registry);
	initSDLInputPlugin(registry, true);
#endif

	initAsioPlugin(registry);
	initHTTPLibPlugin(registry);

#if defined(WITH_DX12) && defined(WITH_DX11)
	if (useDX12) {
		initDX12Plugin(registry);
	} else {
		initDX11Plugin(registry);
	}
#elif defined(WITH_DX12)
	initDX12Plugin(registry);
#elif defined(WITH_DX11)
	initDX11Plugin(registry);
#elif USE_METAL
	initMetalPlugin(registry);
#else
	initOpenGLPlugin(registry);
#endif
	
	return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Network | HalleyAPIFlags::Web;
}

ResourceOptions HalleyEditor::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{
	const auto path = Path(assetsPath) / "editor.dat";
	if (Path::exists(path)) {
		locator.addPack(path, std::nullopt, true);
	} else {
		Logger::logWarning("editor.dat not found, falling back to loading unpacked assets");
		locator.addFileSystem(unpackedAssetsPath);
	}

	return ResourceOptions(true);
}

String HalleyEditor::getName() const
{
	return "Halley Editor";
}

String HalleyEditor::getDataPath(const Vector<String>& args) const
{
	return "halley/editor";
}

bool HalleyEditor::isDevMode() const
{
	return true;
}

bool HalleyEditor::shouldCreateSeparateConsole() const
{
#ifdef _DEBUG
	return isDevMode();
#else
	return false;
#endif
}

bool HalleyEditor::canCollectVideoPerformance()
{
	return false;
}

Preferences& HalleyEditor::getPreferences()
{
	return *preferences;
}

ProjectLoader& HalleyEditor::getProjectLoader()
{
	projectLoader->setDisabledPlatforms(preferences->getDisabledPlatforms());
	return *projectLoader;
}

double HalleyEditor::getTargetBackgroundFPS() const
{
	return 30.0;
}

void HalleyEditor::updateEditor()
{
	getAPI().core->quit(0);
	if (launcherPath && projectPath) {
		OS::get().runCommandDetached(*launcherPath + " --project " + *projectPath);
	}
}

void HalleyEditor::init(const Environment& environment, const Vector<String>& args)
{
	rootPath = environment.getProgramPath().parentPath();

	parseArguments(args);
}

void HalleyEditor::parseArguments(const Vector<String>& args)
{
	enum class ArgType {
		None,
		ProjectPath,
		LauncherPath,
		NoDLL
	};

	ArgType type = ArgType::None;
	projectPath = {};
	launcherPath = {};
	loadDLL = true;

	for (auto& arg : args) {
		if (arg.startsWith("--")) {
			if (arg == "--project") {
				type = ArgType::ProjectPath;
			} else if (arg == "--launcher") {
				type = ArgType::LauncherPath;
			} else if (arg == "--dont-load-dll") {
				type = ArgType::NoDLL;
				loadDLL = false;
			} else if (arg == "--dx12") {
				useDX12 = true;
			}
		} else {
			if (type == ArgType::ProjectPath) {
				if (!projectPath) {
					projectPath = arg;
				} else {
					*projectPath += " " + arg;
				}
			} else if (type == ArgType::LauncherPath) {
				if (!launcherPath) {
					launcherPath = arg;
				} else {
					*launcherPath += " " + arg;
				}
			}
		}
	}
}

std::unique_ptr<Stage> HalleyEditor::startGame()
{
	auto& api = getAPI();

	preferences = std::make_unique<Preferences>();
	preferences->setEditorVersion(getHalleyVersion().toString());
	preferences->loadFromFile(*api.system);
	auto windowDef = preferences->getWindowDefinition();

	projectLoader = std::make_unique<ProjectLoader>(api.core->getStatics(), rootPath, preferences->getDisabledPlatforms());
	std::unique_ptr<Project> project;
	Future<std::unique_ptr<Project>> projectFuture;

	if (projectPath) {
		// NB: this is only safe to execute in another thread because ALL we're doing in this thread in the meanwhile is set up video
		// Both are relatively slow operations, so it makes sense to load project in another thread
		projectFuture = Concurrent::execute([=]
		{
			Logger::logInfo("Loading " + *projectPath);
			return loadProject(Path(*projectPath));
		});
	}

	api.video->setWindow(std::move(windowDef));
	api.video->setVsync(true);
	api.system->setEnableScreensaver(true);

	if (projectPath) {
		project = projectFuture.get();
	}

	return std::make_unique<EditorRootStage>(*this, std::move(project), launcherPath);
}

std::unique_ptr<Project> HalleyEditor::loadProject(Path path)
{
	if (!path.isDirectory()) {
		path = path / ".";
	}
	preferences->applyProjectLoaderPreferences(*projectLoader);
	auto project = projectLoader->loadProject(path);
	if (!project) {
		throw Exception("Unable to load project at " + path.string(), HalleyExceptions::Tools);
	}

	project->loadDLL(getAPI().core->getStatics(), loadDLL);
	project->loadGameResources(getAPI());

	preferences->addRecent(path.string());
	
	return project;
}

std::unique_ptr<Project> HalleyEditor::createProject(Path path)
{
	std::unique_ptr<Project> project;

	// TODO

	if (!project) {
		throw Exception("Unable to create project at " + path.string(), HalleyExceptions::Tools);
	}

	preferences->addRecent(path.string());
	
	return project;
}

HalleyGame(HalleyEditor);
