#include "audio_object_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/tools/project/project.h"
using namespace Halley;

AudioObjectEditor::AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioObject)
{
	factory.loadUI(*this, "halley/audio_editor/audio_object_editor");
}

void AudioObjectEditor::onMakeUI()
{
	hierarchy = getWidgetAs<AudioObjectEditorTreeList>("hierarchy");

	doLoadUI();
}

bool AudioObjectEditor::canParentItemTo(const String& itemId, const String& parentId) const
{
	const auto& item = treeData.at(itemId);
	const auto& parent = treeData.at(parentId);

	if (parent.clip) {
		return false;
	}
	if (item.clip && parent.type != AudioSubObjectType::Clips) {
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
}

std::shared_ptr<const Resource> AudioObjectEditor::loadResource(const String& assetId)
{
	audioObject = std::make_shared<AudioObject>(*gameResources.get<AudioObject>(assetId));
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
		for (const auto& subObject: audioObject->getSubObjects()) {
			populateObject("root", idx++, subObject);
		}
	}
	hierarchy->sortItems();
}

void AudioObjectEditor::populateObject(const String& parentId, size_t idx, const AudioSubObjectHandle& subObject)
{
	if (!subObject.hasValue()) {
		return;
	}

	const bool collapse = false && subObject->canCollapseToClip() && subObject->getClips().size() == 1;
	const auto id = parentId + ":" + toString(idx);

	if (collapse) {
		for (auto& clip: subObject->getClips()) {
			hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(clip), "labelSpecial", factory.makeAssetTypeIcon(AssetType::AudioClip));
			treeData[id] = TreeData{ subObject->getType() };
			break;
		}
	} else {
		// Add this item
		hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(subObject->getName()), "label", makeIcon(subObject->getType()));
		treeData[id] = TreeData{ subObject->getType() };

		// Add sub-categories
		for (auto& cat: subObject->getSubCategories()) {
			const auto catId = id + ":" + cat;
			hierarchy->addTreeItem(catId, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(cat), "label", makeIcon(AudioSubObjectType::None));
			treeData[catId] = TreeData{ subObject->getType(), true };
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
			treeData[clipId] = TreeData{ AudioSubObjectType::None, false, true };
		}
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
