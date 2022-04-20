#include "audio_object_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/tools/project/project.h"
#include "src/scene/choose_window.h"
using namespace Halley;

AudioObjectEditor::AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioObject)
	, projectWindow(projectWindow)
{
	factory.loadUI(*this, "halley/audio_editor/audio_object_editor");
}

void AudioObjectEditor::onMakeUI()
{
	hierarchy = getWidgetAs<AudioObjectEditorTreeList>("hierarchy");

	setHandle(UIEventType::TreeItemReparented, "hierarchy", [=] (const UIEvent& event)
	{
		for (const auto& e: event.getConfigData().asSequence()) {
			moveObject(e["itemId"].asString(), e["parentId"].asString(), e["oldParentId"].asString(), e["childIdx"].asInt(), e["oldChildIdx"].asInt());
		}
	});

	setHandle(UIEventType::ButtonClicked, "add", [=] (const UIEvent& event)
	{
		addObject();
	});

	setHandle(UIEventType::ButtonClicked, "addClip", [=] (const UIEvent& event)
	{
		addClip();
	});

	setHandle(UIEventType::ButtonClicked, "remove", [=] (const UIEvent& event)
	{
		removeCurrentSelection();
	});

	setHandle(UIEventType::ListSelectionChanged, "hierarchy", [=] (const UIEvent& event)
	{
		onSelectionChange(event.getStringData());
	});

	doLoadUI();
}

bool AudioObjectEditor::canDragItem(const String& itemId) const
{
	if (itemId == "root") {
		return false;
	}

	const auto& item = treeData.at(itemId);
	return !item.subCase;
}

bool AudioObjectEditor::canParentItemTo(const String& itemId, const String& parentId) const
{
	if (itemId == "root") {
		return false;
	}
	if (parentId == "root") {
		return true;
	}
	if (parentId == "") {
		return false;
	}

	const auto& item = treeData.at(itemId);
	const auto& parent = treeData.at(parentId);

	if (parent.clip) {
		return false;
	}
	if (item.clip) {
		return (parent.subObject && parent.subObject->getType() == AudioSubObjectType::Clips);
	}
	if (item.subCase) {
		return false;
	}

	return true;
}

void AudioObjectEditor::reload()
{
	doLoadUI();
}

void AudioObjectEditor::refreshAssets()
{
	if (audioObject) {
		audioObject = std::make_shared<AudioObject>(*gameResources.get<AudioObject>(audioObject->getAssetId()));
		doLoadUI();
	}
}

bool AudioObjectEditor::isModified()
{
	return modified;
}

void AudioObjectEditor::save()
{
	if (modified) {
		modified = false;

		const auto assetPath = Path("audio_object/" + audioObject->getAssetId() + ".yaml");
		const auto strData = audioObject->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);
	}
}

void AudioObjectEditor::markModified()
{
	modified = true;
}

void AudioObjectEditor::update(Time t, bool moved)
{
	if (needFullRefresh) {
		doLoadUI();
		needFullRefresh = false;
	}
}

std::shared_ptr<const Resource> AudioObjectEditor::loadResource(const String& assetId)
{
	audioObject = std::make_shared<AudioObject>(*gameResources.get<AudioObject>(assetId));
	audioObject->loadDependencies(gameResources);
	return audioObject;
}

void AudioObjectEditor::doLoadUI()
{
	hierarchy->clear();
	hierarchy->setParent(*this);
	treeData.clear();

	if (audioObject) {
		hierarchy->addTreeItem("root", "", 0, LocalisedString::fromUserString(audioObject->getAssetId()), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioObject));
		size_t idx = 0;
		for (auto& subObject: audioObject->getSubObjects()) {
			populateObject("root", idx++, subObject);
		}
	}
	hierarchy->sortItems();

	layout();
}

void AudioObjectEditor::populateObject(const String& parentId, size_t idx, AudioSubObjectHandle& subObject)
{
	if (!subObject.hasValue()) {
		return;
	}

	const bool collapse = subObject->canCollapseToClip() && subObject->getClips().size() == 1;
	const auto id = parentId + ":" + toString(idx);

	if (collapse) {
		for (auto& clip: subObject->getClips()) {
			hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(clip), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioClip));
			treeData[id] = TreeData{ &subObject.getObject() };
			break;
		}
	} else {
		// Add this item
		hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(subObject->getName()), "label", makeIcon(subObject->getType()));
		treeData[id] = TreeData{ &subObject.getObject() };

		// Add sub-categories
		for (auto& cat: subObject->getSubCategories()) {
			const auto catId = id + ":" + cat;
			hierarchy->addTreeItem(catId, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(cat), "label", makeIcon(AudioSubObjectType::None));
			treeData[catId] = TreeData{ &subObject.getObject(), cat };
		}

		// Populate sub-objects
		const auto n = subObject->getNumSubObjects();
		for (size_t i = 0; i < n; ++i) {
			auto subObjectCat = subObject->getSubObjectCategory(i);
			auto parent = subObjectCat.isEmpty() ? id : (id  + ":" + subObjectCat);
			populateObject(parent, i, subObject->getSubObject(i));
		}

		// Add clips
		for (auto& clip: subObject->getClips()) {
			const auto clipId = id + ":" + clip;
			hierarchy->addTreeItem(clipId, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(clip), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioClip));
			treeData[clipId] = TreeData{ nullptr, {}, clip };
		}
	}
}

void AudioObjectEditor::onSelectionChange(const String& id)
{
	bool canAdd = id == "root";
	bool canAddClip = false;
	const bool canRemove = id != "root";

	const auto iter = treeData.find(id);
	if (iter != treeData.end()) {
		auto& data = iter->second;
		canAdd = data.subObject && data.subObject->canAddObject(data.subCase);
		canAddClip = data.subObject && data.subObject->getType() == AudioSubObjectType::Clips;
	}

	getWidget("add")->setEnabled(canAdd);
	getWidget("addClip")->setEnabled(canAddClip);
	getWidget("remove")->setEnabled(canRemove);
}

void AudioObjectEditor::addObject()
{
	getRoot()->addChild(std::make_shared<ChooseAudioSubObject>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			addObject(fromString<AudioSubObjectType>(*result));
		}
	}));
}

void AudioObjectEditor::addClip()
{
	getRoot()->addChild(std::make_shared<ChooseAssetTypeWindow>(Vector2f(), factory, AssetType::AudioClip, "", gameResources, projectWindow, false, std::optional<String>(), [=](std::optional<String> result)
	{
		if (result) {
			addClip(*result);
		}
	}));
}

void AudioObjectEditor::addObject(AudioSubObjectType type)
{
	auto subObject = AudioSubObjectHandle(IAudioSubObject::makeSubObject(type));
	auto& parent = treeData.at(hierarchy->getSelectedOptionId());
	parent.subObject->addObject(std::move(subObject), parent.subCase, std::numeric_limits<size_t>::max());
	markModified();

	needFullRefresh = true;
}

void AudioObjectEditor::addClip(const String& assetId)
{
	auto clip = gameResources.get<AudioClip>(assetId);
	auto& parent = treeData.at(hierarchy->getSelectedOptionId());
	parent.subObject->addClip(std::move(clip), std::numeric_limits<size_t>::max());
	markModified();

	needFullRefresh = true;
}

void AudioObjectEditor::removeCurrentSelection()
{
	// TODO

	markModified();
	needFullRefresh = true;
}

void AudioObjectEditor::moveItem(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	const auto& item = treeData.at(itemId);
	if (item.clip) {
		moveClip(itemId, parentId, oldParentId, childIdx, oldChildIdx);
	} else {
		moveObject(itemId, parentId, oldParentId, childIdx, oldChildIdx);
	}
	markModified();
}

void AudioObjectEditor::moveObject(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	if (parentId == oldParentId) {
		if (parentId == "root") {
			auto objs = audioObject->getSubObjects();
			std::swap(objs[childIdx], objs[oldChildIdx]);
		} else {
			// This shouldn't happen!
			Logger::logError("Moving audio sub object within same parent, outside of root");
			needFullRefresh = true;
		}
	} else {
		if (parentId == "root") {
			// Move to root
			// TODO
		} else if (oldParentId == "root") {
			// Move from root
			// TODO
		} else {
			// Move between objects
			const auto& parent = treeData.at(parentId);
			const auto& oldParent = treeData.at(oldParentId);
			// TODO
		}

		needFullRefresh = true;
	}
}

void AudioObjectEditor::moveClip(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	// TODO
}

Sprite AudioObjectEditor::makeIcon(AudioSubObjectType type) const
{
	switch (type) {
	case AudioSubObjectType::Layers:
		return Sprite().setImage(factory.getResources(), "ui/audio_object_layers.png");
	case AudioSubObjectType::Sequence:
		return Sprite().setImage(factory.getResources(), "ui/audio_object_sequence.png");
	case AudioSubObjectType::Clips:
		return Sprite().setImage(factory.getResources(), "ui/audio_object_clips.png");
	case AudioSubObjectType::Switch:
		return Sprite().setImage(factory.getResources(), "ui/audio_object_switch.png");
	case AudioSubObjectType::None:
		return Sprite().setImage(factory.getResources(), "ui/audio_object_none.png");
	}
	return Sprite();
}

AudioObjectEditorTreeList::AudioObjectEditorTreeList(String id, UIStyle style)
	: UITreeList(std::move(id), std::move(style))
{
}

void AudioObjectEditorTreeList::setParent(AudioObjectEditor& parent)
{
	this->parent = &parent;
}

bool AudioObjectEditorTreeList::canParentItemTo(const String& itemId, const String& parentId) const
{
	return parent->canParentItemTo(itemId, parentId);
}

bool AudioObjectEditorTreeList::canDragItemId(const String& itemId) const
{
	return parent->canDragItem(itemId);
}

ChooseAudioSubObject::ChooseAudioSubObject(UIFactory& factory, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
{
	Vector<String> ids;
	Vector<String> names;
	for (auto id: EnumNames<AudioSubObjectType>()()) {
		const auto type = fromString<AudioSubObjectType>(id);
		if (type != AudioSubObjectType::None && type != AudioSubObjectType::Clips) {
			ids.push_back(id);
			names.push_back(id);
		}
	}
	setTitle(LocalisedString::fromHardcodedString("Add Audio Sub-object"));
	setAssetIds(ids, names, "switch");
}

void ChooseAudioSubObject::sortItems(Vector<std::pair<String, String>>& values)
{
}
