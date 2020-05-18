#include "project_window.h"


#include "console_window.h"
#include "taskbar.h"
#include "halley/tools/project/project.h"
#include "src/editor_root_stage.h"
#include "src/halley_editor.h"
#include "src/assets/assets_editor_window.h"
#include "src/scene/scene_editor_window.h"

using namespace Halley;

ProjectWindow::ProjectWindow(UIFactory& factory, HalleyEditor& editor, Project& project, Resources& resources, const HalleyAPI& api)
	: UIWidget("project_window", {}, UISizer(UISizerType::Vertical))
	, factory(factory)
	, editor(editor)
	, project(project)
	, resources(resources)
	, api(api)
{
	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.addReloadListener(*this);
	});
	project.addAssetLoadedListener(this);

	tasks = std::make_unique<EditorTaskSet>();
	tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project, false)));

	makeUI();
}

ProjectWindow::~ProjectWindow()
{
	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.removeReloadListener(*this);
	});
	project.removeAssetLoadedListener(this);
}

void ProjectWindow::makeUI()
{
	uiTop = std::make_shared<UIWidget>("uiTop", Vector2f(), UISizer(UISizerType::Horizontal));
	uiMid = std::make_shared<UIWidget>("uiMid", Vector2f(), UISizer(UISizerType::Horizontal));
	uiBottom = std::make_shared<UIWidget>("uiBottom", Vector2f(), UISizer(UISizerType::Horizontal));
	add(uiTop);
	add(uiMid, 1);
	add(uiBottom);

	makeToolbar();
	makePagedPane();
	uiBottom->add(std::make_shared<TaskBar>(factory, *tasks), 1);
}

void ProjectWindow::makeToolbar()
{
	if (toolbar) {
		toolbar->destroy();
		uiTop->clear();
	}
	
	toolbar = std::make_shared<Toolbar>(factory, *this, project);
	
	uiTop->add(toolbar, 1, Vector4f(0, 16, 0, 8));
}

void ProjectWindow::makePagedPane()
{
	if (pagedPane) {
		pagedPane->destroy();
		uiMid->clear();
	}
	
	pagedPane = std::make_shared<UIPagedPane>("pages", numOfStandardTools);
	pagedPane->getPage(static_cast<int>(EditorTabs::Assets))->add(std::make_shared<AssetsEditorWindow>(factory, project, *this), 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Scene))->add(std::make_shared<SceneEditorWindow>(factory, project, api), 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Settings))->add(std::make_shared<ConsoleWindow>(factory), 1, Vector4f(8, 8, 8, 8));

	uiMid->add(pagedPane, 1);
}

bool ProjectWindow::loadCustomUI()
{
	destroyCustomUI();

	auto game = project.createGameInstance();
	if (!game) {
		return false;
	}
	
	auto customToolsInterface = game->createEditorCustomToolsInterface();
	if (!customToolsInterface) {
		return false;
	}
	
	try {
		customTools = customToolsInterface->makeTools(IEditorCustomTools::MakeToolArgs(factory, resources, project.getGameResources(), api));
	} catch (const std::exception& e) {
		Logger::logException(e);
	} catch (...) {
		return false;
	}

	if (!customTools.empty()) {
		toolbar->getList()->add(std::make_shared<UIImage>(Sprite().setImage(resources, "ui/slant_capsule_short.png").setColour(Colour4f::fromString("#FC2847"))), 0, Vector4f(0, 3, 0, 3));
		
		for (auto& tool: customTools) {
			const auto img = std::make_shared<UIImage>(tool.icon);
			toolbar->getList()->addImage(tool.id, img, 1, {}, UISizerAlignFlags::Centre);
			pagedPane->addPage()->add(tool.widget, 1, Vector4f(8, 8, 8, 8));
		}
	}

	return true;
}

void ProjectWindow::destroyCustomUI()
{
	if (!customTools.empty()) {
		makeToolbar();
		customTools.clear();
	}
	pagedPane->resizePages(numOfStandardTools);
}

void ProjectWindow::onLoadDLL()
{
	waitingToLoadCustomUI = true;
}

void ProjectWindow::onUnloadDLL()
{
	destroyCustomUI();
}

void ProjectWindow::onAssetsLoaded()
{
	if (waitingToLoadCustomUI) {
		if (loadCustomUI()) {
			waitingToLoadCustomUI = false;
		}
	}
}

void ProjectWindow::update(Time t, bool moved)
{
	if (tasks) {
		tasks->update(t);
	}

	const auto size = api.video->getWindow().getDefinition().getSize();
	setMinSize(Vector2f(size));
}

void ProjectWindow::setPage(EditorTabs tab)
{
	pagedPane->setPage(static_cast<int>(tab));
}

String ProjectWindow::setCustomPage(const String& pageId)
{
	int i = 0;
	for (auto& custom: customTools) {
		if (custom.id == pageId) {
			pagedPane->setPage(numOfStandardTools + i);
			return custom.text;
		}
		++i;
	}
	return "???";
}

void ProjectWindow::openPrefab(const String& name, AssetType assetType)
{
	auto sceneEditor = getWidgetAs<SceneEditorWindow>("scene_editor");
	if (assetType == AssetType::Scene) {
		sceneEditor->loadScene(name);
	} else if (assetType == AssetType::Prefab) {
		sceneEditor->loadPrefab(name);
	}
	toolbar->getList()->setSelectedOption(static_cast<int>(EditorTabs::Scene));
}

EditorTaskSet& ProjectWindow::getTasks() const
{
	return *tasks;
}
