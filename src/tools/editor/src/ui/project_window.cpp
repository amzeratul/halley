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
		hasDLL = dll.isLoaded();
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

	setHandle(UIEventType::NavigateTo, [=] (const UIEvent& event)
	{
		auto uri = event.getStringData();
		if (uri.startsWith("asset:")) {
			auto splitURI = uri.split(':');
			if (splitURI.size() == 3) {
				toolbar->getList()->setSelectedOptionId(toString(EditorTabs::Assets));
				assetEditorWindow->showAsset(fromString<AssetType>(splitURI.at(1)), splitURI.at(2));
			}
		}
	});
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

	assetEditorWindow = std::make_shared<AssetsEditorWindow>(factory, project, *this);
	sceneEditorTabs = std::make_shared<SceneEditorTabs>(factory, project, api);
	consoleWindow = std::make_shared<ConsoleWindow>(factory);
	
	pagedPane = std::make_shared<UIPagedPane>("pages", numOfStandardTools);
	pagedPane->getPage(static_cast<int>(EditorTabs::Assets))->add(assetEditorWindow, 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Scene))->add(sceneEditorTabs, 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(static_cast<int>(EditorTabs::Settings))->add(consoleWindow, 1, Vector4f(8, 8, 8, 8));

	uiMid->add(pagedPane, 1);
}

void ProjectWindow::tryLoadCustomUI()
{
	if (waitingToLoadCustomUI && hasAssets && hasDLL) {
		if (loadCustomUI()) {
			waitingToLoadCustomUI = false;
		}
	}
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
			toolbar->getList()->getItem(tool.id)->setToolTip(tool.tooltip);
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
	hasDLL = true;
	waitingToLoadCustomUI = true;
	tryLoadCustomUI();
}

void ProjectWindow::onUnloadDLL()
{
	destroyCustomUI();
	for (const auto& ss: resources.enumerate<SpriteSheet>()) {
		resources.get<SpriteSheet>(ss)->clearMaterialCache();
	}
}

void ProjectWindow::onAssetsLoaded()
{
	hasAssets = true;
	tryLoadCustomUI();
	for (auto& c: customTools) {
		c.widget->sendEvent(UIEvent(UIEventType::AssetsReloaded, getId()));
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

LocalisedString ProjectWindow::setCustomPage(const String& pageId)
{
	int i = 0;
	for (auto& custom: customTools) {
		if (custom.id == pageId) {
			pagedPane->setPage(numOfStandardTools + i);
			return custom.text;
		}
		++i;
	}
	return LocalisedString::fromHardcodedString("???");
}

void ProjectWindow::openPrefab(const String& name, AssetType assetType)
{
	sceneEditorTabs->load(assetType, name);
	toolbar->getList()->setSelectedOption(static_cast<int>(EditorTabs::Scene));
}

EditorTaskSet& ProjectWindow::getTasks() const
{
	return *tasks;
}
