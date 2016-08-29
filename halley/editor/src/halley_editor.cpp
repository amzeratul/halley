#include "halley_editor.h"
#include "editor_root_stage.h"
#include "halley/tools/project/project.h"
#include "preferences.h"
#include "halley/core/game/environment.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);

HalleyEditor::HalleyEditor()
{
}

HalleyEditor::~HalleyEditor()
{
}

int HalleyEditor::initPlugins(IPluginRegistry &registry)
{
	if (headless) {
		return 0;
	} else {
		initOpenGLPlugin(registry);
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input;
	}
}

void HalleyEditor::initResourceLocator(String dataPath, ResourceLocator& locator)
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
	//return !headless && isDevBuild();
	return false;
}

void HalleyEditor::init(const Environment& environment, const Vector<String>& args)
{
	using boost::filesystem::path;
	sharedAssetsPath = path(environment.getProgramPath().cppStr()).parent_path().parent_path() / "assets_src";

	preferences = std::make_unique<Preferences>((path(environment.getDataPath().cppStr()) / "settings.yaml").string());
	preferences->load();

	parseArguments(args);
}

void HalleyEditor::parseArguments(const std::vector<String>& args)
{
	using boost::filesystem::path;

	headless = false;
	bool gotProjectPath = false;

	for (auto& arg : args) {
		if (arg.startsWith("--")) {
			if (arg == "--headless") {
				headless = true;
			} else {
				std::cout << "Unknown argument \"" << arg << "\".\n";
			}
		} else {
			if (!gotProjectPath) {
				loadProject(path(arg.cppStr()));
				gotProjectPath = true;
			} else {
				std::cout << "Unknown argument \"" << arg << "\".\n";
			}
		}
	}
}

std::unique_ptr<Stage> HalleyEditor::startGame(HalleyAPI* api)
{
	if (!headless) {
		Rect4i rect = preferences->getWindowRect();
		Vector2i winSize(1280, 720);
		Vector2i winPos = api->video->getCenteredWindow(winSize, 0);
		api->video->setWindow(Window(WindowType::ResizableWindow, winPos, winSize, getName()), false);
	}
	return std::make_unique<EditorRootStage>(*this);
}

Project& HalleyEditor::loadProject(boost::filesystem::path path)
{
	project = std::make_unique<Project>(path, sharedAssetsPath);

	if (!project) {
		throw Exception("Unable to load project at " + path.string());
	}
	preferences->addRecent(path.string());
	return *project;
}

Project& HalleyEditor::createProject(boost::filesystem::path path)
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
