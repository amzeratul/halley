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
}

void ProjectWindow::makeUI()
{
	uiTop = std::make_shared<UIWidget>("uiTop", Vector2f(), UISizer(UISizerType::Horizontal));
	uiMid = std::make_shared<UIWidget>("uiMid", Vector2f(), UISizer(UISizerType::Horizontal));
	uiBottom = std::make_shared<UIWidget>("uiBottom", Vector2f(), UISizer(UISizerType::Horizontal));
	add(uiTop);
	add(uiMid, 1);
	add(uiBottom);

	pagedPane = std::make_shared<UIPagedPane>("pages", 6);
	toolbar = std::make_shared<Toolbar>(factory, *this, project);
	const auto taskbar = std::make_shared<TaskBar>(factory, *tasks);

	uiTop->add(toolbar, 1, Vector4f(0, 16, 0, 8));
	uiMid->add(pagedPane, 1);
	uiBottom->add(taskbar, 1);

	pagedPane->getPage(static_cast<int>(EditorTabs::Assets))->add(std::make_shared<AssetsEditorWindow>(factory, project, *this), 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Scene))->add(std::make_shared<SceneEditorWindow>(factory, project, api), 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Settings))->add(std::make_shared<ConsoleWindow>(factory), 1, Vector4f(8, 8, 8, 8));

	loadCustomProjectUI();
}

void ProjectWindow::loadCustomProjectUI()
{
	destroyCustomProjectUI();

	auto game = project.createGameInstance();
	if (game) {
		auto customToolsInterface = game->createEditorCustomToolsInterface();
		if (customToolsInterface) {
			customTools = customToolsInterface->makeTools(IEditorCustomTools::MakeToolArgs(resources, project.getGameResources(), api));

			for (auto& tool: customTools) {
				// TODO: create
			}
		}
	}
}

void ProjectWindow::destroyCustomProjectUI()
{
	for (auto& tool: customTools) {
		// TODO: destroy
	}
	customTools.clear();
}

void ProjectWindow::onLoadDLL()
{
	loadCustomProjectUI();
}

void ProjectWindow::update(Time t, bool moved)
{
	if (tasks) {
		tasks->update(t);
	}

	const auto size = api.video->getWindow().getDefinition().getSize();
	setMinSize(Vector2f(size));
}

void ProjectWindow::onUnloadDLL()
{
	destroyCustomProjectUI();
}

void ProjectWindow::setPage(EditorTabs tab)
{
	pagedPane->setPage(int(tab));
}

void ProjectWindow::openPrefab(const String& name, AssetType assetType)
{
	auto sceneEditor = getWidgetAs<SceneEditorWindow>("scene_editor");
	if (assetType == AssetType::Scene) {
		sceneEditor->loadScene(name);
	} else if (assetType == AssetType::Prefab) {
		sceneEditor->loadPrefab(name);
	}
	auto toolbar = getWidgetAs<UIList>("toolbarList");
	toolbar->setSelectedOption(int(EditorTabs::Scene));
	//pagedPane->setPage(int(EditorTabs::Scene));
}

EditorTaskSet& ProjectWindow::getTasks() const
{
	return *tasks;
}
