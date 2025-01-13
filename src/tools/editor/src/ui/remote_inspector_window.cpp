#include "remote_inspector_window.h"

using namespace Halley;

RemoteInspectorWindow::RemoteInspectorWindow(UIFactory& factory, std::shared_ptr<DevConServerConnection> connection, const HalleyAPI& api)
	: UIWidget("remote_inspector_window", {}, UISizer())
	, factory(factory)
	, connection(std::move(connection))
	, api(api)
{
	inspectorServer = std::make_shared<InspectorServer>(this->connection);
	inspectorServer->setWorldInfoCallback([=](const Vector<InspectorWorldInfo>& infos)
	{
		onWorldInfo(infos);
	});
	inspectorServer->setWorldDataCallback([=](const InspectorWorldData& data)
	{
		onWorldData(data);
	});

	factory.loadUI(*this, "halley/remote_inspector_window");
}

void RemoteInspectorWindow::onMakeUI()
{
	setHandle(UIEventType::DropdownSelectionChanged, "worlds", [=] (const UIEvent& event)
	{
		setWorld(event.getStringData());
	});
}

void RemoteInspectorWindow::onActiveChanged(bool active)
{
	inspectorServer->setListening(active);
}

void RemoteInspectorWindow::update(Time t, bool moved)
{
	ConfigNode params;
	params["world"] = curWorld;
	params["entity"] = curEntity;
	inspectorServer->setParams(std::move(params));
}

void RemoteInspectorWindow::onWorldInfo(const Vector<InspectorWorldInfo>& infos)
{
	Vector<UIDropdown::Entry> entries;
	for (auto& info: infos) {
		entries.push_back(UIDropdown::Entry(info.uuid.toString(), LocalisedString::fromUserString(info.name)));
	}

	auto dropdown = getWidgetAs<UIDropdown>("worlds");
	auto prevSel = dropdown->getSelectedOptionId();
	dropdown->setOptions(std::move(entries));
	dropdown->setSelectedOption(prevSel);
	setWorld(dropdown->getSelectedOptionId());
}

void RemoteInspectorWindow::onWorldData(const InspectorWorldData& worldData)
{
	auto entityList = getWidgetAs<UITreeList>("entityList");

	entityList->clear();
	for (const auto& entityInfo : worldData.entities) {
		entityList->addTreeItem(entityInfo.id.toString(), entityInfo.parentId.toString(), std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(entityInfo.name));
	}
}

void RemoteInspectorWindow::setWorld(const String& id)
{
	curWorld = UUID::tryParse(id).value_or(UUID());
}
