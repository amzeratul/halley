#include "move_files_tool.h"

#include "halley/audio/sub_objects/audio_sub_object_clips.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project.h"

MoveFilesTool::MoveFilesTool(UIFactory& factory, UIFactory& editorFactory, Project& project, Vector<ConfigBreadCrumb> configBreadCrumbs)
	: UIWidget("move_files_tool", {}, UISizer())
	, factory(factory)
	, editorFactory(editorFactory)
	, project(project)
	, monitor(project.getAssetsSrcPath())
	, configBreadCrumbs(std::move(configBreadCrumbs))
{
	factory.loadUI(*this, "editor/utils/move_files_util");
	setStage(Stage::Monitoring);
	setAnchor({});

	project.setAssetSaveNotification(false);

	setupComponentReplacementTable();
}

MoveFilesTool::~MoveFilesTool()
{
	project.setAssetSaveNotification(true);
}

void MoveFilesTool::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "next", [=] (const UIEvent& event)
	{
		onNext();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		destroy();
	});

	log = getWidgetAs<UIList>("log");
}

void MoveFilesTool::update(Time t, bool moved)
{
	const auto curChanges = monitor.poll();

	if (curStage == Stage::Monitoring) {
		for (const auto& change: curChanges) {
			if (change.type == DirectoryMonitor::ChangeType::FileRenamed) {
				addRename(change.oldName, change.name, change.isDir);
			}
			if (change.type == DirectoryMonitor::ChangeType::FileRemoved) {
				lastRemove = change.name;
			}
			if (change.type == DirectoryMonitor::ChangeType::FileAdded && lastRemove) {
				if (Path(change.name).getFilename() == Path(*lastRemove).getFilename()) {
					addRename(*lastRemove, change.name, change.isDir);
				} else {
					Logger::logInfo("Mismatch, seemingly renaming " + *lastRemove + " to " + change.name);
				}
				lastRemove = {};
			}
		}
	} else if (curStage == Stage::ComputingChanges) {
		receiveChangedFiles();

		if (computing.isReady()) {
			setStage(Stage::ConfirmChanges);
		}
	}

	if (logDirty) {
		populateLog();
		logDirty = false;
	}
}

void MoveFilesTool::onNext()
{
	setStage(static_cast<Stage>(static_cast<int>(curStage) + 1));
}

void MoveFilesTool::setStage(Stage stage)
{
	curStage = stage;

	auto status = getWidgetAs<UILabel>("status");
	auto next = getWidgetAs<UIButton>("next");

	if (stage == Stage::Monitoring) {
		status->setText(LocalisedString::fromHardcodedString("Waiting for files to be moved..."));
		next->setLabel(LocalisedString::fromHardcodedString("Done moving"));
	} else if (stage == Stage::ComputingChanges) {
		status->setText(LocalisedString::fromHardcodedString("Finding files that need updating..."));
		next->setLabel(LocalisedString::fromHardcodedString("Please wait..."));
		next->setEnabled(false);
		startComputingChanges();
	} else if (stage == Stage::ConfirmChanges) {
		status->setText(LocalisedString::fromHardcodedString("Update these files?"));
		next->setLabel(LocalisedString::fromHardcodedString("Confirm"));
		next->setEnabled(true);
		doneComputingChanges();
	} else if (stage == Stage::Done) {
		confirmChanges();
		destroy();
	}
}

void MoveFilesTool::addRename(String oldName, String newName, bool isDir)
{
	const auto src = project.getAssetsSrcPath();
	filesMoved.push_back(MovedFile{
		Path(oldName).makeRelativeTo(src).toString(),
		Path(newName).makeRelativeTo(src).toString(),
		isDir,
	});
	logDirty = true;
}

void MoveFilesTool::populateLog()
{
	// Post-process to collapse chains
	for (int i = static_cast<int>(filesMoved.size()); --i >= 0; ) {
		if (i > 0) {
			for (int j = i; --j >= 0;) {
				if (filesMoved[j].to == filesMoved[i].from) {
					filesMoved[i].from = filesMoved[j].from;
					filesMoved.erase(filesMoved.begin() + j);
					--i;
					if (j > 0) {
						--j;
					}
				}
			}
		}

		if (filesMoved[i].from == filesMoved[i].to) {
			filesMoved.erase(filesMoved.begin() + i);
			if (i > 0) {
				--i;
			}
		}
	}

	log->clear();
	for (const auto& change: filesMoved) {
		auto icon = editorFactory.makeImportAssetTypeIcon(project.getImportAssetType(change.to));
		log->addTextIconItem("", LocalisedString::fromUserString(change.from + " -> " + change.to), std::move(icon));
	}
}

void MoveFilesTool::receiveChangedFiles()
{
	ChangeList newChanges;
	{
		auto lock = std::unique_lock<std::mutex>(toChangeMutex);
		newChanges = std::move(toChangeOutbox);
		toChangeOutbox = {};
	}
	addChangesToLog(newChanges);

	for (auto& c: newChanges) {
		toChange.emplace_back(std::move(c));
	}
}

void MoveFilesTool::addChangesToLog(const ChangeList& entries)
{
	for (const auto& entry: entries) {
		auto icon = editorFactory.makeImportAssetTypeIcon(project.getImportAssetType(entry.first));
		log->addTextIconItem("", LocalisedString::fromUserString(entry.first.toString()), std::move(icon));
	}
}

void MoveFilesTool::startComputingChanges()
{
	log->clear();
	computing = Concurrent::execute([this] ()
	{
		computeChanges();
	});
}

void MoveFilesTool::doneComputingChanges()
{
	if (toChange.empty()) {
		log->addTextItem("", LocalisedString::fromHardcodedString("No assets found to modify"));
	}
}

void MoveFilesTool::confirmChanges()
{
	for (const auto& entry: toChange) {
		project.writeAssetToDisk(entry.first, entry.second);
	}
	toChange.clear();
}

void MoveFilesTool::addChangedFile(Path filePath, String data)
{
	Bytes result;
	result.resize(data.length());
	memcpy(result.data(), data.c_str(), data.length());
	addChangedFile(std::move(filePath), std::move(result));
}

void MoveFilesTool::addChangedFile(Path filePath, Bytes data)
{
	auto lock = std::unique_lock<std::mutex>(toChangeMutex);
	toChangeOutbox.emplace_back(std::move(filePath), std::move(data));
}

void MoveFilesTool::computeChanges()
{
	MovedFilesByType movedFilesByType;
	for (auto& file: filesMoved) {
		const auto type = project.getImportAssetType(file.to);

		const auto srcPath = Path(file.from).dropFront(1);
		const auto dstPath = Path(file.to).dropFront(1);

		auto file1 = file;
		file1.from = srcPath.toString();
		file1.to = dstPath.toString();
		movedFilesByType[type].push_back(file1);

		const auto ext = dstPath.getExtension();
		if (type == ImportAssetType::Sprite && (ext == ".ase" || ext == ".aseprite")) {
			auto file2 = file;
			file2.from = srcPath.replaceExtension("").toString();
			file2.to = dstPath.replaceExtension("").toString();
			movedFilesByType[type].push_back(file2);
		}
	}

	updateGameplayConfig(movedFilesByType, Path("config") / "gameplay");
	updatePrefabs(movedFilesByType, "prefab");
	updateScenes(movedFilesByType, "scene");
	updateUI(movedFilesByType, "ui");
	updateAudioEvents(movedFilesByType, "audio_event");
	updateAudioObjects(movedFilesByType, "audio_object");
}

void MoveFilesTool::updateGameplayConfig(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path : OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		auto bytes = project.readAssetFromDisk(rootPath / path);
		auto config = YAMLConvert::parseConfig(bytes);
		auto changes = findConfigChanges(movedFiles, config.getRoot());
		if (!changes.empty()) {
			auto lines = String(reinterpret_cast<const char*>(bytes.data()), bytes.size()).split('\n');
			applyChangesToLines(changes, lines);
			addChangedFile(rootPath / path, String::concatList(lines, "\n"));
		}
	}
}

void MoveFilesTool::updatePrefabs(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path: OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		Prefab prefab;
		prefab.parseYAML(project.readAssetFromDisk(rootPath / path).byte_span());

		if (updatePrefab(movedFiles, prefab)) {
			addChangedFile(rootPath / path, prefab.toYAML());
		}
	}
}

void MoveFilesTool::updateScenes(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path: OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		Scene scene;
		scene.parseYAML(project.readAssetFromDisk(rootPath / path).byte_span());

		if (updatePrefab(movedFiles, scene)) {
			addChangedFile(rootPath / path, scene.toYAML());
		}
	}
}

void MoveFilesTool::updateAudioEvents(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path: OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		AudioEvent audioEvent;
		audioEvent.parseYAML(project.readAssetFromDisk(rootPath / path).byte_span());

		if (updateAudioEvent(movedFiles, audioEvent)) {
			addChangedFile(rootPath / path, audioEvent.toYAML());
		}
	}
}

void MoveFilesTool::updateAudioObjects(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path: OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		AudioObject audioObject;
		audioObject.parseYAML(project.readAssetFromDisk(rootPath / path).byte_span());

		if (updateAudioObject(movedFiles, audioObject)) {
			addChangedFile(rootPath / path, audioObject.toYAML());
		}
	}
}

void MoveFilesTool::updateUI(const MovedFilesByType& movedFiles, const Path& rootPath)
{
	for (const auto& path: OS::get().enumerateDirectory(project.getAssetsSrcPath() / rootPath)) {
		UIDefinition ui;
		ui.parseYAML(project.readAssetFromDisk(rootPath / path).byte_span());

		if (updateUI(movedFiles, ui)) {
			addChangedFile(rootPath / path, ui.toYAML());
		}
	}
}

bool MoveFilesTool::updatePrefab(const MovedFilesByType& movedFiles, Prefab& prefab)
{
	bool changed = false;
	for (auto& data: prefab.getEntityDatas()) {
		changed = updateEntityData(movedFiles, data) || changed;
	}
	return changed;
}

bool MoveFilesTool::updateEntityData(const MovedFilesByType& movedFiles, EntityData& data)
{
	bool changed = false;

	for (auto& [compName, compData]: data.getComponents()) {
		changed = updateComponentData(movedFiles, compName, compData) || changed;
	}

	for (auto& c: data.getChildren()) {
		changed = updateEntityData(movedFiles, c) || changed;
	}
	return changed;
}

bool MoveFilesTool::updateComponentData(const MovedFilesByType& movedFiles, const String& componentName, ConfigNode& componentData)
{
	bool changed = false;

	if (componentName == "SpriteAnimation") {
		if (componentData.hasKey("player")) {
			auto& playerData = componentData["player"];
			if (playerData.hasKey("animation")) {
				changed = updateConfigNode(movedFiles, playerData["animation"], ImportAssetType::Sprite) || changed;
			}
		}
	}

	if (componentName == "Sprite") {
		if (componentData.hasKey("sprite")) {
			auto& spriteData = componentData["sprite"];
			for (auto& [k, v]: spriteData.asMap()) {
				changed = updateConfigNode(movedFiles, v, ImportAssetType::Sprite) || changed;
			}
		}
	}

	if (componentName == "Particles") {
		if (componentData.hasKey("sprites")) {
			auto& spritesData = componentData["sprites"];
			for (auto& spriteData : spritesData.asSequence()) {
				for (auto& [k, v] : spriteData.asMap()) {
					changed = updateConfigNode(movedFiles, v, ImportAssetType::Sprite) || changed;
				}
			}
		}
	}

	if (auto compIter = componentReplacementTable.find(componentName); compIter != componentReplacementTable.end()) {
		for (const auto& [fieldName, importAssetType, isVector]: compIter->second) {
			if (componentData.hasKey(fieldName)) {
				if (isVector) {
					auto& s = componentData[fieldName].asSequence();
					for (auto& e: s) {
						changed = updateConfigNode(movedFiles, e, importAssetType) || changed;
					}
				} else {
					changed = updateConfigNode(movedFiles, componentData[fieldName], importAssetType) || changed;
				}
			}
		}
	}

	return changed;
}

bool MoveFilesTool::updateConfigNode(const MovedFilesByType& movedFiles, ConfigNode& node, ImportAssetType type)
{
	if (node.getType() != ConfigNodeType::String) {
		return false;
	}

	const auto iter = movedFiles.find(type);
	if (iter == movedFiles.end()) {
		return false;
	}
	const auto& files = iter->second;

	if (auto result = updateString(files, node.asString())) {
		node = std::move(*result);
		return true;
	}
	return false;
}

std::optional<String> MoveFilesTool::updateString(const Vector<MovedFile>& movedFiles, const String& str)
{
	bool modified = false;
	auto split = str.split(":");

	for (const auto& file: movedFiles) {
		if (file.isDir) {
			if (split[0].startsWith(file.from)) {
				split[0] = file.to + split[0].mid(file.from.length());
				modified = true;
			}
		} else {
			if (split[0] == file.from) {
				split[0] = file.to;
				modified = true;
			}
		}
	}

	if (modified) {
		return String::concatList(split, ":");
	} else {
		return {};
	}
}

bool MoveFilesTool::updateUI(const MovedFilesByType& movedFiles, UIDefinition& ui)
{
	return updateUINode(movedFiles, ui.getRoot());
}

bool MoveFilesTool::updateUINode(const MovedFilesByType& movedFiles, ConfigNode& uiNode)
{
	bool modified = false;
	if (uiNode.hasKey("widget")) {
		modified = updateUIWidget(movedFiles, uiNode["widget"]) || modified;
	}
	if (uiNode.hasKey("children")) {
		for (auto& c: uiNode["children"].asSequence()) {
			modified = updateUINode(movedFiles, c) || modified;
		}
	}
	return modified;
}

bool MoveFilesTool::updateUIWidget(const MovedFilesByType& movedFiles, ConfigNode& widgetNode)
{
	if (!widgetNode.hasKey("class")) {
		return false;
	}

	bool modified = false;
	const auto classId = widgetNode["class"].asString();
	if (classId == "image") {
		if (widgetNode.hasKey("image")) {
			modified = updateConfigNode(movedFiles, widgetNode["image"], ImportAssetType::Sprite) || modified;
		}
	}

	return modified;
}

bool MoveFilesTool::updateAudioEvent(const MovedFilesByType& movedFiles, AudioEvent& event)
{
	const auto iter = movedFiles.find(ImportAssetType::AudioObject);
	if (iter == movedFiles.end()) {
		return false;
	}
	const auto& objects = iter->second;

	bool modified = false;

	for (auto& action: event.getActions()) {
		if (auto* objectAction = dynamic_cast<AudioEventActionObject*>(action.get())) {
			if (auto updated = updateString(objects, objectAction->getObjectName())) {
				objectAction->setObjectName(*updated);
				modified = true;
			}
		}
	}
	return modified;
}

bool MoveFilesTool::updateAudioObject(const MovedFilesByType& movedFiles, AudioObject& object)
{
	const auto iter = movedFiles.find(ImportAssetType::AudioClip);
	if (iter == movedFiles.end()) {
		return false;
	}
	const auto& clips = iter->second;

	bool modified = false;
	updateAudioObject(clips, object, modified);

	return modified;
}

void MoveFilesTool::updateAudioObject(const Vector<MovedFile>& movedFiles, IAudioObject& object, bool& modified)
{
	if (auto* clips = dynamic_cast<AudioSubObjectClips*>(&object)) {
		for (auto& clip: clips->getClips()) {
			if (auto updated = updateString(movedFiles, clip)) {
				clip = *updated;
				modified = true;
			}
		}
	}

	const auto n = object.getNumSubObjects();
	for (size_t i = 0; i < n; ++i) {
		updateAudioObject(movedFiles, *object.getSubObject(i), modified);
	}
}

void MoveFilesTool::setupComponentReplacementTable()
{
	auto parseType = [] (String str) -> std::optional<ImportAssetType>
	{
		if (str.startsWith("Halley::")) {
			str = str.mid(8);
		}
		str = str.left(1).asciiLower() + str.mid(1);
		return tryFromString<ImportAssetType>(str);
	};

	for (const auto& component: project.getECSData().getComponents()) {
		for (const auto& member: component.second.members) {
			std::optional<ImportAssetType> type;
			bool vector = false;
			if (member.type.name.startsWith("Halley::ResourceReference<")) {
				type = parseType(member.type.name.mid(26, member.type.name.size() - 27));
			} else if (member.type.name.startsWith("Halley::Vector<Halley::ResourceReference<")) {
				type = parseType(member.type.name.mid(41, member.type.name.size() - 43));
				vector = true;
			}

			if (type) {
				componentReplacementTable[component.first].emplace_back(member.name, *type, vector);
			}
		}
	}
}

MoveFilesTool::ConfigBreadCrumbRef::ConfigBreadCrumbRef(const ConfigBreadCrumb& crumb)
	: breadCrumb(crumb.breadCrumb.span())
	, assetType(crumb.assetType)
{
}

MoveFilesTool::ConfigBreadCrumbRef::ConfigBreadCrumbRef(gsl::span<const String> breadCrumb, ImportAssetType assetType)
	: breadCrumb(breadCrumb)
	, assetType(assetType)
{
}

MoveFilesTool::ConfigBreadCrumbRef MoveFilesTool::ConfigBreadCrumbRef::dropFront() const
{
	return ConfigBreadCrumbRef{ breadCrumb.subspan(1), assetType };
}

bool MoveFilesTool::ConfigBreadCrumbRef::empty() const
{
	return breadCrumb.empty();
}

bool MoveFilesTool::ConfigBreadCrumbRef::startsWith(const String& value) const
{
	return !empty() && breadCrumb[0] == value;
}

Vector<MoveFilesTool::ConfigChange> MoveFilesTool::findConfigChanges(const MovedFilesByType& movedFiles, const ConfigNode& config)
{
	// Get crumbs and filter by those affecting file types that were modified
	Vector<ConfigBreadCrumbRef> crumbRefs;
	for (auto& crumb: configBreadCrumbs) {
		if (movedFiles.contains(crumb.assetType)) {
			crumbRefs.push_back(crumb);
		}
	}

	Vector<ConfigChange> changes;
	doFindConfigChanges(changes, movedFiles, crumbRefs, config);
	return changes;
}

void MoveFilesTool::doFindConfigChanges(Vector<ConfigChange>& changes, const MovedFilesByType& movedFiles, const Vector<ConfigBreadCrumbRef>& breadCrumbs, const ConfigNode& config)
{
	if (config.getType() == ConfigNodeType::Map) {
		for (const auto& [k, v]: config.asMap()) {
			Vector<ConfigBreadCrumbRef> newCrumbs;
			for (const auto& crumb: breadCrumbs) {
				if (crumb.startsWith(k) || crumb.startsWith("*")) {
					newCrumbs.push_back(crumb.dropFront());
				}
			}
			if (!newCrumbs.empty()) {
				doFindConfigChanges(changes, movedFiles, newCrumbs, v);
			}
		}
	} else if (config.getType() == ConfigNodeType::Sequence) {
		for (const auto& v: config.asSequence()) {
			Vector<ConfigBreadCrumbRef> newCrumbs;
			for (const auto& crumb: breadCrumbs) {
				if (crumb.startsWith("#")) {
					newCrumbs.push_back(crumb.dropFront());
				}
			}
			if (!newCrumbs.empty()) {
				doFindConfigChanges(changes, movedFiles, newCrumbs, v);
			}
		}
	} else {
		for (const auto& crumb: breadCrumbs) {
			if (crumb.empty()) {
				// Found an interest point
				checkConfigChanges(changes, movedFiles, config, crumb.assetType);
				break;
			}
		}
	}
}

void MoveFilesTool::checkConfigChanges(Vector<ConfigChange>& changes, const MovedFilesByType& movedFiles, const ConfigNode& config, ImportAssetType type)
{
	if (config.getType() == ConfigNodeType::String) {
		auto origStr = config.asString();
		if (auto result = updateString(movedFiles.at(type), origStr)) {
			const auto pos = config.getOriginalPosition();
			changes.push_back(ConfigChange{ pos.first, pos.second, std::move(origStr), std::move(*result) });
		}
	}
}

void MoveFilesTool::applyChangesToLines(const Vector<ConfigChange>& changes, Vector<String>& lines)
{
	for (const auto& change: changes) {
		auto& line = lines.at(change.line);

		line = line.replaceOne(change.from, change.to);
	}
}
