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
	hierarchy = getWidgetAs<UITreeList>("hierarchy");

	doLoadUI();
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
	if (audioObject) {
		hierarchy->addTreeItem("root", "", 0, LocalisedString::fromHardcodedString("Root"));
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

	// Add this item
	const auto id = parentId + ":" + toString(idx);
	hierarchy->addTreeItem(id, parentId, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(subObject->getName()), "label", Sprite());

	// Add sub-categories
	for (auto& cat: subObject->getSubCategories()) {
		hierarchy->addTreeItem(id + ":" + cat, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(cat));
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
		hierarchy->addTreeItem(id + ":" + clip, id, std::numeric_limits<size_t>::max(), LocalisedString::fromUserString(clip), "labelSpecial", Sprite());
	}
}
