#include "halley_editor.h"
#include "editor_root_stage.h"
#include "halley/tools/project/project.h"
#include "preferences.h"
#include "halley/core/game/environment.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry);
void initAsioPlugin(IPluginRegistry &registry);

HalleyEditor::HalleyEditor()
{
}

HalleyEditor::~HalleyEditor()
{
}

int HalleyEditor::initPlugins(IPluginRegistry &registry)
{
	initSDLSystemPlugin(registry);
	initAsioPlugin(registry);
	if (headless) {
		return HalleyAPIFlags::Network;
	} else {
		initSDLAudioPlugin(registry);
		initSDLInputPlugin(registry);
		initOpenGLPlugin(registry);
		
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Network;
	}
}

void HalleyEditor::initResourceLocator(Path dataPath, ResourceLocator& locator)
{
	locator.addFileSystem(dataPath);
}

String HalleyEditor::getName() const
{
	return "Halley Editor";
}

String HalleyEditor::getDataPath() const
{
	return "halley/editor";
}

bool HalleyEditor::isDevBuild() const
{
	return true;
}

bool HalleyEditor::shouldCreateSeparateConsole() const
{
#ifdef _DEBUG
	return !headless && isDevBuild();
#else
	return false;
#endif
}

void HalleyEditor::init(const Environment& environment, const Vector<String>& args)
{
	rootPath = environment.getProgramPath().parentPath();

	preferences = std::make_unique<Preferences>((environment.getDataPath() / "settings.yaml").string());
	preferences->load();

	parseArguments(args);
}

void HalleyEditor::parseArguments(const std::vector<String>& args)
{
	headless = false;
	platform = "pc";
	gotProjectPath = false;

	for (auto& arg : args) {
		if (arg.startsWith("--")) {
			if (arg == "--headless") {
				headless = true;
			} if (arg.startsWith("--platform=")) {
				platform = arg.mid(String("--platform=").length());
			} else {
				std::cout << "Unknown argument \"" << arg << "\".\n";
			}
		} else {
			if (!gotProjectPath) {
				projectPath = arg.cppStr();
				gotProjectPath = true;
			} else {
				std::cout << "Unknown argument \"" << arg << "\".\n";
			}
		}
	}
}

std::unique_ptr<Stage> HalleyEditor::startGame(const HalleyAPI* api)
{
	if (gotProjectPath) {
		loadProject(api->core->getStatics(), platform, Path(projectPath));
	} else {
		throw Exception("Please specify a project path.");
	}

	if (!headless) {
		Vector2i winSize(1280, 720);
		api->video->setWindow(WindowDefinition(WindowType::ResizableWindow, winSize, getName()), true);
	}
	return std::make_unique<EditorRootStage>(*this);
}

Project& HalleyEditor::loadProject(const HalleyStatics& statics, const String& activePlatform, Path path)
{
	project = std::make_unique<Project>(statics, activePlatform, path, rootPath);

	if (!project) {
		throw Exception("Unable to load project at " + path.string());
	}
	preferences->addRecent(path.string());
	return *project;
}

Project& HalleyEditor::createProject(Path path)
{
	project.reset();

	if (!project) {
		throw Exception("Unable to create project at " + path.string());
	}
	preferences->addRecent(path.string());
	return *project;
}

bool HalleyEditor::hasProjectLoaded() const
{
	return static_cast<bool>(project);
}

Project& HalleyEditor::getProject() const
{
	if (!project) {
		throw Exception("No project loaded.");
	}
	return *project;
}

HalleyGame(HalleyEditor);
