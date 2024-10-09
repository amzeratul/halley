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
void initAsioPlugin(IPluginRegistry &registry);
void initDX11Plugin(IPluginRegistry &registry);
void initMetalPlugin(IPluginRegistry &registry);

HalleyEditor::HalleyEditor()
{
}

HalleyEditor::~HalleyEditor()
{
}

int HalleyEditor::initPlugins(IPluginRegistry &registry)
{
	initSDLSystemPlugin(registry, {});
	initAsioPlugin(registry);
	initSDLAudioPlugin(registry);
	initSDLInputPlugin(registry, true);

#ifdef _WIN32
	initDX11Plugin(registry);
#elif __APPLE__
	initMetalPlugin(registry);
#else
	initOpenGLPlugin(registry);
#endif
	
	return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Network;
}

ResourceOptions HalleyEditor::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{
	locator.addFileSystem(unpackedAssetsPath);
	return ResourceOptions(true);
}

String HalleyEditor::getName() const
{
	return "Halley Editor";
}

String HalleyEditor::getDataPath() const
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
		LauncherPath
	};

	ArgType type = ArgType::None;
	projectPath = {};
	launcherPath = {};

	for (auto& arg : args) {
		if (arg.startsWith("--")) {
			if (arg == "--project") {
				type = ArgType::ProjectPath;
			} else if (arg == "--launcher") {
				type = ArgType::LauncherPath;
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

	projectLoader = std::make_unique<ProjectLoader>(api.core->getStatics(), rootPath, preferences->getDisabledPlatforms());
	std::unique_ptr<Project> project;

	if (projectPath) {
		Logger::logInfo("Loading " + *projectPath);
		project = loadProject(Path(*projectPath));
	}

	api.video->setWindow(preferences->getWindowDefinition());
	api.video->setVsync(true);
	return std::make_unique<EditorRootStage>(*this, std::move(project), launcherPath);
}

std::unique_ptr<Project> HalleyEditor::loadProject(Path path)
{
	if (!path.isDirectory()) {
		path = path / ".";
	}
	auto project = projectLoader->loadProject(path);
	if (!project) {
		throw Exception("Unable to load project at " + path.string(), HalleyExceptions::Tools);
	}

	project->loadDLL(getAPI().core->getStatics());
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
