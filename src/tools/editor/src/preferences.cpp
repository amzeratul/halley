#include "preferences.h"
using namespace Halley;

Preferences::Preferences(SystemAPI& system) 
	: system(system)
	, windowSize(Vector2i(1280, 720))
{
	loadFromFile();
}

ConfigNode Preferences::save() const
{
	ConfigNode::MapType root;

	{
		ConfigNode::SequenceType recentsNode;
		for (auto& r: recents) {
			recentsNode.push_back(String(r));
		}
		root["recents"] = std::move(recentsNode);
	}

	{
		ConfigNode::MapType windowNode;
		if (windowPosition) {
			windowNode["position"] = windowPosition.get();
		}
		windowNode["size"] = windowSize;
		windowNode["state"] = int(windowState);
		root["window"] = std::move(windowNode);
	}

	return std::move(root);
}

void Preferences::load(const ConfigNode& root)
{
	recents.clear();
	windowPosition = {};
	windowSize = Vector2i(1280, 720);
	windowState = WindowState::Normal;

	if (root.hasKey("recents")) {
		for (auto& r: root["recents"]) {
			recents.push_back(r.asString());
		}
	}
	if (root.hasKey("window")) {
		auto& windowNode = root["window"];
		if (windowNode.hasKey("position")) {
			windowPosition = windowNode["position"].asVector2i();
		}
		windowSize = windowNode["size"].asVector2i();
		windowState = WindowState(windowNode["state"].asInt());
	}
}

bool Preferences::isDirty() const
{
	return dirty;
}

void Preferences::saveToFile() const
{
	ConfigFile file;
	file.getRoot() = save();
	system.getStorageContainer(SaveDataType::SaveLocal)->setData("preferences", Serializer::toBytes(file));
	dirty = false;
}

void Preferences::loadFromFile()
{
	auto data = system.getStorageContainer(SaveDataType::SaveLocal)->getData("preferences");
	if (!data.empty()) {
		auto config = Deserializer::fromBytes<ConfigFile>(data);
		load(config.getRoot());
	}
}

void Preferences::addRecent(Path path)
{
	auto name = path.string();
	recents.erase(std::remove(recents.begin(), recents.end(), name), recents.end());
	recents.insert(recents.begin(), name);
	dirty = true;
}

const std::vector<String>& Preferences::getRecents() const
{
	return recents;
}

WindowDefinition Preferences::getWindowDefinition() const
{
	return WindowDefinition(WindowType::ResizableWindow, windowPosition, windowSize, "Halley Editor").withState(windowState);
}

void Preferences::updateWindowDefinition(const Window& window)
{
	auto rect = window.getWindowRect();
	auto state = window.getDefinition().getWindowState();
	auto size = rect.getSize();
	auto pos = rect.getTopLeft();

	if (windowState != state) {
		windowState = state;
		dirty = true;
	}

	if (state == WindowState::Normal && (windowPosition != pos || windowSize != size)) {
		windowPosition = pos;
		windowSize = size;
		dirty = true;
	}
}
