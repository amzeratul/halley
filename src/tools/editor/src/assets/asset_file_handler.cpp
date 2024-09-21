#include "asset_file_handler.h"

#include "halley/audio/audio_object.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/project.h"

using namespace Halley;

AssetFileHandler::AssetFileHandler()
{
	populate();
}

const IAssetFileHandler* AssetFileHandler::tryGetHandlerFor(const String& assetType) const
{
	if (assetType.isEmpty()) {
		return nullptr;
	}
	const auto iter = handlers.find(assetType);
	if (iter != handlers.end()) {
		return iter->second.get();
	}
	return nullptr;
}

const IAssetFileHandler* AssetFileHandler::tryGetHandlerFor(const Path& path) const
{
	const auto& pathParts = path.getParts();
	if (pathParts.size() >= 2 && pathParts[0] == ".." && pathParts[1] == "halley") {
		return pathParts.size() >= 4 ? tryGetHandlerFor(pathParts[3]) : nullptr;
	} else {
		return !pathParts.empty() ? tryGetHandlerFor(pathParts[0]) : nullptr;
	}
}

void AssetFileHandler::populate()
{
	clear();
	addHandler(std::make_unique<AssetFileHandlerPrefab>());
	addHandler(std::make_unique<AssetFileHandlerScene>());
	addHandler(std::make_unique<AssetFileHandlerAudioEvent>());
	addHandler(std::make_unique<AssetFileHandlerAudioObject>());
	addHandler(std::make_unique<AssetFileHandlerCometScript>());
	addHandler(std::make_unique<AssetFileHandlerUI>());
}

void AssetFileHandler::clear()
{
	handlers.clear();
}

void AssetFileHandler::addHandler(std::unique_ptr<IAssetFileHandler> handler)
{
	handlers[handler->getRootAssetDirectory()] = std::move(handler);
}

AssetFileHandlerBase::AssetFileHandlerBase(AssetType type, String rootAssetDir, String name, String fileExtension)
	: type(type)
	, rootAssetDir(std::move(rootAssetDir))
	, name(std::move(name))
	, fileExtension(std::move(fileExtension))
{}

AssetType AssetFileHandlerBase::getAssetType() const
{
	return type;
}

std::string_view AssetFileHandlerBase::getRootAssetDirectory() const
{
	return rootAssetDir;
}

std::string_view AssetFileHandlerBase::getName() const
{
	return name;
}

std::string_view AssetFileHandlerBase::getFileExtension() const
{
	return fileExtension;
}

Vector<IAssetFileHandler::MenuEntry> AssetFileHandlerBase::getContextMenuEntries() const
{
	return {};
}

void AssetFileHandlerBase::onContextMenu(const String& actionId, UIRoot& ui, EditorUIFactory& factory, const String& assetId, Project& project) const
{
}

Vector<IAssetFileHandler::MenuEntry> AssetFileHandlerBase::getEmptySpaceContextMenuEntries() const
{
	return {};
}

AssetFileHandlerPrefab::AssetFileHandlerPrefab()
	: AssetFileHandlerBase(AssetType::Prefab, "prefab", "Prefab", ".prefab")
{}

String AssetFileHandlerPrefab::makeDefaultFile() const
{
	Prefab prefab;
	prefab.makeDefault();
	return prefab.toYAML();
}

String AssetFileHandlerPrefab::duplicateAsset(const ConfigNode& node) const
{
	Prefab prefab;
	prefab.parseConfigNode(node);
	prefab.generateUUIDs();
	return prefab.toYAML();
}

AssetFileHandlerScene::AssetFileHandlerScene()
	: AssetFileHandlerBase(AssetType::Scene, "scene", "Scene", ".scene")
{}

String AssetFileHandlerScene::makeDefaultFile() const
{
	Scene scene;
	scene.makeDefault();
	return scene.toYAML();	
}

String AssetFileHandlerScene::duplicateAsset(const ConfigNode& node) const
{
	Scene scene;
	scene.parseConfigNode(node);
	scene.generateUUIDs();
	return scene.toYAML();
}

AssetFileHandlerAudioObject::AssetFileHandlerAudioObject()
	: AssetFileHandlerBase(AssetType::AudioObject, "audio_object", "Audio Object", ".audioobject")
{}

String AssetFileHandlerAudioObject::makeDefaultFile() const
{
	AudioObject audioObject;
	audioObject.makeDefault();
	return audioObject.toYAML();
}

String AssetFileHandlerAudioObject::duplicateAsset(const ConfigNode& node) const
{
	return AudioObject(node).toYAML();
}

AssetFileHandlerAudioEvent::AssetFileHandlerAudioEvent()
	: AssetFileHandlerBase(AssetType::AudioEvent, "audio_event", "Audio Event", ".audioevent")
{}

String AssetFileHandlerAudioEvent::makeDefaultFile() const
{
	AudioEvent audioEvent;
	audioEvent.makeDefault();
	return audioEvent.toYAML();
}

String AssetFileHandlerAudioEvent::duplicateAsset(const ConfigNode& node) const
{
	return AudioEvent(node).toYAML();
}

Vector<IAssetFileHandler::MenuEntry> AssetFileHandlerAudioEvent::getEmptySpaceContextMenuEntries() const
{
	return {
		MenuEntry { "convertAllLegacy", "Convert All Legacy Events", "Convert all legacy events, creating AudioObjects for them.", "" }
	};
}

void AssetFileHandlerAudioEvent::onContextMenu(const String& actionId, UIRoot& ui, EditorUIFactory& factory, const String& assetId, Project& project) const
{
	if (actionId == "convertAllLegacy") {
		convertLegacyEvents(project);
	}
}

void AssetFileHandlerAudioEvent::convertLegacyEvents(Project& project) const
{
	Logger::logInfo("Converting all legacy events...");

	auto& fs = project.getFileSystemCache();

	auto rootDir = project.getAssetsSrcPath() / "audio_event";
	auto objectDir = project.getAssetsSrcPath() / "audio_object";

	for (auto& f: fs.enumerateDirectory(rootDir)) {
		auto eventPath = rootDir / f;
		auto id = f.replaceExtension("").getString();

		auto data = YAMLConvert::parseConfig(fs.readFile(eventPath));
		auto newObjects = AudioEvent::convertLegacy(id, data.getRoot());

		if (!newObjects.empty()) {
			Logger::logInfo("+ Found legacy: " + id);
			bool allGood = true;

			for (auto& o: newObjects) {
				const auto objectPath = (objectDir / o.getAssetId()).replaceExtension(".audioobject");
				if (fs.exists(objectPath)) {
					Logger::logError("- AudioObject already exists: " + o.getAssetId());
					allGood = false;
				}
			}

			if (allGood) {
				for (auto& o: newObjects) {
					const auto objectPath = (objectDir / o.getAssetId()).replaceExtension(".audioobject");
					fs.writeFile(objectPath, o.toYAML());
				}
				fs.writeFile(eventPath, YAMLConvert::generateYAML(data));
			}
		}
	}

	Logger::logInfo("Done");
}

AssetFileHandlerUI::AssetFileHandlerUI()
	: AssetFileHandlerBase(AssetType::UIDefinition, "ui", "UI", ".ui")
{}

String AssetFileHandlerUI::makeDefaultFile() const
{
	UIDefinition ui;
	ui.makeDefault();
	return ui.toYAML();	
}

String AssetFileHandlerUI::duplicateAsset(const ConfigNode& node) const
{
	return UIDefinition(ConfigNode(node)).toYAML();
}

AssetFileHandlerCometScript::AssetFileHandlerCometScript()
	: AssetFileHandlerBase(AssetType::ScriptGraph, "comet", "Comet Script", ".comet")
{}

String AssetFileHandlerCometScript::makeDefaultFile() const
{
	ScriptGraph script;
	script.makeDefault();
	return script.toYAML();
}

String AssetFileHandlerCometScript::duplicateAsset(const ConfigNode& node) const
{
	return ScriptGraph(node).toYAML();
}

