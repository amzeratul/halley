#include "asset_file_handler.h"

#include "halley/audio/audio_object.h"

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

bool AssetFileHandler::canAdd(std::string_view rootAssetDir) const
{
	return addFolders.contains(rootAssetDir);
}

bool AssetFileHandler::canDuplicate(std::string_view rootAssetDir) const
{
	return duplicateFolders.contains(rootAssetDir);
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
	addFolders.clear();
	duplicateFolders.clear();
}

void AssetFileHandler::addHandler(std::unique_ptr<IAssetFileHandler> handler)
{
	if (handler->canCreateNew()) {
		addFolders.insert(handler->getRootAssetDirectory());
	}
	if (handler->canDuplicate()) {
		duplicateFolders.insert(handler->getRootAssetDirectory());
	}
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

