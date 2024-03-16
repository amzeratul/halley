#include "preferences.h"
using namespace Halley;

Preferences::Preferences() 
{
	loadDefaults();
}

void Preferences::setEditorVersion(String editorVersion)
{
	this->editorVersion = std::move(editorVersion);
}

void Preferences::loadDefaults()
{
	recents.clear();
	windowPosition = {};
	windowSize = Vector2i(1280, 720);
	windowState = WindowState::Normal;
	colourScheme = "Flat Halley (Dark)";
	lz4hc = false;
	autoBuild = false;
	canEditHalleyAssets = false;
}

ConfigNode Preferences::save() const
{
	ConfigNode::MapType root;

	root["recents"] = ConfigNode(recents);

	{
		ConfigNode::MapType windowNode;
		if (windowPosition) {
			windowNode["position"] = windowPosition.value();
		}
		windowNode["size"] = windowSize;
		windowNode["state"] = int(windowState);
		root["window"] = std::move(windowNode);
	}

	root["disabledPlatforms"] = ConfigNode(disabledPlatforms);
	root["colourScheme"] = colourScheme;
	root["lz4hc"] = lz4hc;
	root["autoBuild"] = autoBuild;
	root["canEditHalleyAssets"] = canEditHalleyAssets;

	return root;
}

void Preferences::load(const ConfigNode& root)
{
	loadDefaults();

	recents = root["recents"].asVector<String>({});
	if (root.hasKey("window")) {
		auto& windowNode = root["window"];
		if (windowNode.hasKey("position")) {
			windowPosition = windowNode["position"].asVector2i();
		}
		windowSize = windowNode["size"].asVector2i();
		windowState = WindowState(windowNode["state"].asInt());
	}
	disabledPlatforms = root["disabledPlatforms"].asVector<String>({});

	if (root.hasKey("colourScheme")) {
		colourScheme = root["colourScheme"].asString();
	}
	lz4hc = root["lz4hc"].asBool(false);
	autoBuild = root["autoBuild"].asBool(false);
	canEditHalleyAssets = root["canEditHalleyAssets"].asBool(false);
}

bool Preferences::isDirty() const
{
	return dirty;
}

void Preferences::saveToFile(SystemAPI& system) const
{
	ConfigFile file;
	file.getRoot() = save();
	system.getStorageContainer(SaveDataType::SaveLocal)->setData("preferences", Serializer::toBytes(file));
	dirty = false;
}

void Preferences::loadFromFile(SystemAPI& system)
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

const Vector<String>& Preferences::getRecents() const
{
	return recents;
}

WindowDefinition Preferences::getWindowDefinition() const
{
	return WindowDefinition(WindowType::ResizableWindow, windowPosition, windowSize, "Halley Editor - " + editorVersion, true).withState(windowState);
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

const Vector<String>& Preferences::getDisabledPlatforms() const
{
	return disabledPlatforms;
}

bool Preferences::isPlatformDisabled(const String& name) const
{
	return std::find(disabledPlatforms.begin(), disabledPlatforms.end(), name) != disabledPlatforms.end();
}

void Preferences::setPlatformDisabled(const String& name, bool disabled)
{
	dirty = true;
	
	const auto iter = std::find(disabledPlatforms.begin(), disabledPlatforms.end(), name);
	const bool curDisabled = iter != disabledPlatforms.end();
	
	if (curDisabled != disabled) {
		if (disabled) {
			disabledPlatforms.push_back(name);
		} else {
			disabledPlatforms.erase(iter);
		}
	}
}

const String& Preferences::getColourScheme() const
{
	return colourScheme;
}

void Preferences::setColourScheme(String colourScheme)
{
	this->colourScheme = std::move(colourScheme);
}

bool Preferences::isLZ4HCEnabled() const
{
	return lz4hc;
}

void Preferences::setLZ4HCEnabled(bool enabled)
{
	lz4hc = enabled;
}

bool Preferences::isAutoBuild() const
{
	return autoBuild;
}

void Preferences::setAutoBuild(bool enabled)
{
	autoBuild = enabled;
}

void Preferences::loadEditorPreferences(const Preferences& preferences)
{
	dirty = true;
	disabledPlatforms = preferences.disabledPlatforms;
	colourScheme = preferences.colourScheme;
	lz4hc = preferences.lz4hc;
	autoBuild = preferences.autoBuild;
	canEditHalleyAssets = preferences.canEditHalleyAssets;
}

void Preferences::applyProjectLoaderPreferences(ProjectLoader& projectLoader)
{
	projectLoader.setImporterOption("lz4hc", ConfigNode(lz4hc));
}

bool Preferences::getCanEditHalleyAssets() const
{
	return canEditHalleyAssets;
}

void Preferences::setCanEditHalleyAssets(bool enabled)
{
	canEditHalleyAssets = enabled;
}
