#include "audio_object_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/tools/project/project.h"
using namespace Halley;

AudioObjectEditor::AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioObject)
{
}

void AudioObjectEditor::reload()
{
	// TODO
}

void AudioObjectEditor::refreshAssets()
{
	// TODO
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
	// TODO
}

std::shared_ptr<const Resource> AudioObjectEditor::loadResource(const String& assetId)
{
	audioObject = std::make_shared<AudioObject>(*gameResources.get<AudioObject>(assetId));
	return audioObject;
}
