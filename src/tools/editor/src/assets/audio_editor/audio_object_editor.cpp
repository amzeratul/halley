#include "audio_object_editor.h"

#include "audio_clips_editor.h"
#include "audio_layers_editor.h"
#include "audio_root_editor.h"
#include "audio_sequence_editor.h"
#include "audio_switch_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/audio/sub_objects/audio_sub_object_clips.h"
#include "halley/tools/project/project.h"
#include "src/scene/choose_window.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"
#include "halley/audio/sub_objects/audio_sub_object_sequence.h"
#include "halley/audio/sub_objects/audio_sub_object_switch.h"
#include "halley/core/properties/game_properties.h"
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
			moveItem(e["itemId"].asString(), e["parentId"].asString(), e["oldParentId"].asString(), e["childIdx"].asInt(), e["oldChildIdx"].asInt());
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
		return (item.clip || (item.object && item.object->getType() == AudioSubObjectType::Clips));
	}
	if (item.clip) {
		return (parent.object && parent.object->canAddObject(AudioSubObjectType::Clips, parent.subCase));
	}
	if (item.subCase) {
		return false;
	}

	return parent.object->canAddObject(item.object->getType(), parent.subCase);
}

const AudioProperties& AudioObjectEditor::getAudioProperties() const
{
	return project.getGameProperties().getAudioProperties();
}

void AudioObjectEditor::reload()
{
	doLoadUI();
}

void AudioObjectEditor::refreshAssets()
{
	// if (audioObject) {
	// 	audioObject = std::make_shared<AudioObject>(*gameResources.get<AudioObject>(audioObject->getAssetId()));
	// 	doLoadUI();
	// }
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

std::shared_ptr<const Resource> AudioObjectEditor::loadResource(const String& assetId)
{
	const auto assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(assetType, assetId);
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);

	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		audioObject = std::make_shared<AudioObject>(config.getRoot());
	} else {
		audioObject = std::make_shared<AudioObject>();
		markModified(false);
	}
	audioObject->setAssetId(assetId);

	setCurrentObject(audioObject.get());
	
	return audioObject;
}

void AudioObjectEditor::markModified(bool refreshList)
{
	modified = true;
	if (refreshList) {
		needFullRefresh = true;
	}
}

void AudioObjectEditor::update(Time t, bool moved)
{
	if (needFullRefresh) {
		doLoadUI();
		needFullRefresh = false;
	}
}

void AudioObjectEditor::doLoadUI()
{
	hierarchy->setCanSendEvents(false);
	auto prevId = hierarchy->getSelectedOptionId();
	hierarchy->clear();
	hierarchy->setParent(*this);
	treeData.clear();

	if (audioObject) {
		hierarchy->addTreeItem("root", "", 0, LocalisedString::fromUserString(audioObject->getAssetId()), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioObject));
		treeData["root"] = TreeData{ "", audioObject.get()};

		size_t idx = 0;
		for (auto& subObject: audioObject->getSubObjects()) {
			populateObject("root", idx++, subObject);
		}
	}
	hierarchy->sortItems();
	hierarchy->refresh();
	hierarchy->layout();
	hierarchy->setSelectedOptionId(prevId);
	hierarchy->setCanSendEvents(true);

	doSetCurrentObject();
}

void AudioObjectEditor::populateObject(const String& parentId, size_t idx, AudioSubObjectHandle& subObject)
{
	if (!subObject.hasValue()) {
		return;
	}

	const bool collapse = parentId != "root" && subObject->canCollapseToClip() && subObject->getClips().size() == 1;
	const auto id = parentId + ":" + toString(idx);

	if (collapse) {
		for (auto& clip: subObject->getClips()) {
			hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(clip), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioClip));
			treeData[id] = TreeData{ parentId, &subObject.getObject() };
			break;
		}
	} else {
		// Add this item
		hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(subObject->getName()), "label", makeIcon(subObject->getType()));
		treeData[id] = TreeData{ parentId, &subObject.getObject() };

		// Add sub-categories
		for (auto& cat: subObject->getSubCategories(getAudioProperties())) {
			const auto catId = id + ":" + cat;
			hierarchy->addTreeItem(catId, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(cat), "label", makeIcon(AudioSubObjectType::None));
			treeData[catId] = TreeData{ id, &subObject.getObject(), cat };
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
			treeData[clipId] = TreeData{ id, nullptr, {}, clip };
		}
	}
}

void AudioObjectEditor::onSelectionChange(const String& id)
{
	auto& data = treeData.at(id);

	const bool canAdd = data.object && data.object->canAddObject(AudioSubObjectType::Switch, data.subCase);
	const bool canAddClip = data.object && data.object->canAddObject(AudioSubObjectType::Clips, data.subCase);
	const bool canRemove = id != "root" && !data.subCase;

	getWidget("add")->setEnabled(canAdd);
	getWidget("addClip")->setEnabled(canAddClip);
	getWidget("remove")->setEnabled(canRemove);

	if (data.object) {
		setCurrentObject(data.object);
	}
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
	parent.object->addObject(std::move(subObject), parent.subCase, std::numeric_limits<size_t>::max());
	markModified(true);
}

void AudioObjectEditor::addClip(const String& assetId)
{
	auto clip = gameResources.get<AudioClip>(assetId);
	auto& parent = treeData.at(hierarchy->getSelectedOptionId());
	parent.object->addClip(std::move(clip), {}, std::numeric_limits<size_t>::max());
	markModified(true);
}

void AudioObjectEditor::removeCurrentSelection()
{
	auto& target = treeData.at(hierarchy->getSelectedOptionId());
	auto& parent = treeData.at(target.parent);
	if (target.clip) {
		parent.object->removeClip(*target.clip);
	} else {
		parent.object->removeObject(target.object);
	}
	setCurrentObject(nullptr);

	markModified(true);
}

PRAGMA_DEOPTIMIZE
void AudioObjectEditor::moveItem(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	const auto& item = treeData.at(itemId);
	if (item.clip) {
		moveClip(itemId, parentId, oldParentId, childIdx, oldChildIdx);
	} else {
		moveObject(itemId, parentId, oldParentId, childIdx, oldChildIdx);
	}
	markModified(false);
}

void AudioObjectEditor::moveObject(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	if (parentId == oldParentId) {
		const auto& parent = treeData.at(parentId);
		auto& a = parent.object->getSubObject(childIdx);
		auto& b = parent.object->getSubObject(oldChildIdx);
		std::swap(a, b);
	} else {
		const auto& item = treeData.at(itemId);
		const auto& parent = treeData.at(parentId);
		const auto& oldParent = treeData.at(oldParentId);

		auto obj = oldParent.object->removeObject(item.object);
		parent.object->addObject(std::move(obj), parent.subCase, childIdx);

		needFullRefresh = true;
	}
}

void AudioObjectEditor::moveClip(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx)
{
	if (parentId == oldParentId) {
		const auto& parent = treeData.at(parentId);
		parent.object->swapClips(childIdx, oldChildIdx);
	} else {
		const auto& item = treeData.at(itemId);
		const auto& parent = treeData.at(parentId);
		const auto& oldParent = treeData.at(oldParentId);

		oldParent.object->removeClip(item.clip.value());
		parent.object->addClip(gameResources.get<AudioClip>(item.clip.value()), parent.subCase, childIdx);

		needFullRefresh = true;
	}
}

void AudioObjectEditor::setCurrentObject(IAudioObject* object)
{
	if (currentObject != object) {
		currentObject = object;

		doSetCurrentObject();
	}
}

void AudioObjectEditor::doSetCurrentObject()
{
	if (!currentObject) {
		setCurrentObjectEditor({});
		return;
	}

	switch (currentObject->getType()) {
	case AudioSubObjectType::None:
		// Root
		setCurrentObjectEditor(std::make_shared<AudioRootEditor>(factory, *this, dynamic_cast<AudioObject&>(*currentObject)));
		break;
	case AudioSubObjectType::Clips:
		setCurrentObjectEditor(std::make_shared<AudioClipsEditor>(factory, *this, dynamic_cast<AudioSubObjectClips&>(*currentObject)));
		break;
	case AudioSubObjectType::Layers:
		setCurrentObjectEditor(std::make_shared<AudioLayersEditor>(factory, *this, dynamic_cast<AudioSubObjectLayers&>(*currentObject)));
		break;
	case AudioSubObjectType::Sequence:
		setCurrentObjectEditor(std::make_shared<AudioSequenceEditor>(factory, *this, dynamic_cast<AudioSubObjectSequence&>(*currentObject)));
		break;
	case AudioSubObjectType::Switch:
		setCurrentObjectEditor(std::make_shared<AudioSwitchEditor>(factory, *this, dynamic_cast<AudioSubObjectSwitch&>(*currentObject)));
		break;
	}
}

void AudioObjectEditor::setCurrentObjectEditor(std::shared_ptr<UIWidget> widget)
{
	if (currentObjectEditor) {
		currentObjectEditor->destroy();
	}
	currentObjectEditor = std::move(widget);
	if (currentObjectEditor) {
		getWidget("contents")->add(currentObjectEditor, 1);
	}
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
