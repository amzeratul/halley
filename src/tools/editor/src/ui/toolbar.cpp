#include "toolbar.h"

#include "project_window.h"
#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"
#include "halley/ui/ui_sizer.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/text/string_converter.h"

using namespace Halley;

Toolbar::Toolbar(UIFactory& factory, ProjectWindow& projectWindow, Project& project)
	: UIWidget("toolbar", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, project(project)
{
	makeUI();
}

const std::shared_ptr<UIList>& Toolbar::getList() const
{
	return list;
}

void Toolbar::onPageSet(const String& tabId)
{
	list->setSelectedOptionId(tabId);
}

void Toolbar::makeUI()
{
	add(factory.makeUI("halley/toolbar"), 1);

	getWidgetAs<UILabel>("gameName")->setText(LocalisedString::fromUserString(project.getProperties().getName()));
	list = getWidgetAs<UIList>("toolbarList");
	list->setFocusable(false);

	auto platformDropdown = getWidgetAs<UIDropdown>("platform");
	auto buildPlatforms = project.getBuildPlatforms();
	platformDropdown->setActive(buildPlatforms.size() >= 2);
	if (buildPlatforms.size() >= 2) {
		Vector<UIDropdown::Entry> entries;
		for (const auto platform: buildPlatforms) {
			auto icon = Sprite().setImage(factory.getResources(), "ui/platforms/" + toString(platform) + "_16.png");
			entries.emplace_back(UIDropdown::Entry{ toString(platform), LocalisedString::fromUserString(getPlatformName(platform)), std::move(icon) });
		}
		platformDropdown->setOptions(std::move(entries));
	}

	setHandle(UIEventType::ListSelectionChanged, "toolbarList", [=] (const UIEvent& event)
	{
		String toolName;
		const auto& tabId = event.getStringData();
		const auto& toolNameWidget = getWidgetAs<UILabel>("toolName");

		auto tabNames = EnumNames<EditorTabs>()();
		if (std::find_if(tabNames.begin(), tabNames.end(), [&](const char* v) { return tabId == v; }) == tabNames.end()) {
			const auto name = projectWindow.setCustomPage(tabId);
			toolNameWidget->setText(name);
			return;
		}
		
		const auto tab = fromString<EditorTabs>(tabId);
		
		switch (tab) {
		case EditorTabs::Assets:
			toolName = "Assets";
			break;
		case EditorTabs::ECS:
			toolName = "ECS";
			break;
		case EditorTabs::Remotes:
			toolName = "Remotes";
			break;
		case EditorTabs::Localisation:
			toolName = "Localisation";
			break;
		case EditorTabs::Properties:
			toolName = "Properties";
			break;
		case EditorTabs::Settings:
			toolName = "Settings";
			break;
		case EditorTabs::Terminal:
			toolName = "Terminal";
			break;
		case EditorTabs::Plot:
			toolName = "Plotter";
			break;
		}
		toolNameWidget->setText(LocalisedString::fromHardcodedString(toolName));
		projectWindow.setPage(tab);
	});
	
	setHandle(UIEventType::ButtonClicked, "exitProject", [=] (const UIEvent& event)
	{
		projectWindow.closeProject();
	});

	setHandle(UIEventType::ButtonClicked, "runProject", [=] (const UIEvent& event)
	{
		project.launchGame(projectWindow.getLaunchArguments());
	});

	setHandle(UIEventType::ButtonClicked, "buildProject", [=] (const UIEvent& event)
	{
		projectWindow.buildGame();
	});

	setHandle(UIEventType::ButtonClicked, "import", [=] (const UIEvent& event)
	{
		auto menuOptions = Vector<UIPopupMenuItem>();
		
		menuOptions.push_back(UIPopupMenuItem("ImportAll", LocalisedString::fromHardcodedString("Import Assets"), {}, LocalisedString::fromHardcodedString("Import all assets")));
		menuOptions.push_back(UIPopupMenuItem("ReimportAll", LocalisedString::fromHardcodedString("Re-Import All Assets"), {}, LocalisedString::fromHardcodedString("Reimport all assets from scratch")));
		menuOptions.push_back(UIPopupMenuItem("Codegen", LocalisedString::fromHardcodedString("Re-Run Codegen"), {}, LocalisedString::fromHardcodedString("Re-run codegen")));

		auto menu = std::make_shared<UIPopupMenu>("asset_browser_context_menu", factory.getStyle("popupMenu"), menuOptions);
		menu->spawnOnRoot(*getRoot());

		menu->setHandle(UIEventType::PopupAccept, [this] (const UIEvent& e) {
			project.requestReimport(fromString<ReimportType>(e.getStringData()));
		});
	});

	setHandle(UIEventType::DropdownSelectionChanged, "platform", [=] (const UIEvent& event)
	{
		project.setTargetPlatform(fromString<GamePlatform>(event.getStringData()));
	});
}

String Toolbar::getPlatformName(GamePlatform platform) const
{
	switch (platform) {
	case GamePlatform::Windows:
		return "Windows";
	case GamePlatform::MacOS:
		return "macOS";
	case GamePlatform::Linux:
		return "Linux";
	case GamePlatform::Switch:
		return "Switch";
	case GamePlatform::XboxOne:
		return "Xbox One";
	case GamePlatform::XboxSeries:
		return "Xbox Series";
	case GamePlatform::PS4:
		return "PS4";
	case GamePlatform::PS5:
		return "PS5";
	case GamePlatform::UWP:
		return "UWP";
	case GamePlatform::Android:
		return "Android";
	case GamePlatform::iOS:
		return "iOS";
	case GamePlatform::Emscripten:
		return "Emscripten";
	case GamePlatform::FreeBSD:
		return "FreeBSD";
	default:
		return toString(platform);
	}
}
