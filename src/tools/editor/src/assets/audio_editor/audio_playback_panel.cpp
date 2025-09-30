#include "audio_playback_panel.h"

#include "halley/tools/project/project.h"

using namespace Halley;

AudioPlaybackPanel::AudioPlaybackPanel(UIFactory& factory, const HalleyAPI& api, Project& project)
	: UIWidget("audio_playback", {}, UISizer())
	, factory(factory)
	, api(api)
	, project(project)
{
	factory.loadUI(*this, "halley/audio_editor/audio_playback_panel");
}

void AudioPlaybackPanel::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "play", [=] (const UIEvent& event) {
		onPlay();
	});
}

void AudioPlaybackPanel::update(Time t, bool moved)
{
	if (needsIconUpdate) {
		const char* image = isPlaying() ? "halley_ui/icon_pause.png" : "halley_ui/icon_play.png";
		getWidgetAs<UIButton>("play")->setIcon(Sprite().setImage(factory.getResources(), image));
	}

	if (needsObjectUpdate && updateCooldown <= 0) {
		needsObjectUpdate = false;
		updateCooldown = 0.1;
		updatePlaybackObject();
	}
	if (updateCooldown > 0) {
		updateCooldown -= t;
	}
}

void AudioPlaybackPanel::onActiveChanged(bool active)
{
	if (!active) {
		pause();
	}
}

void AudioPlaybackPanel::onObjectModified()
{
	if (object) {
		needsObjectUpdate = true;
	}
}

void AudioPlaybackPanel::setAudioObject(std::shared_ptr<const AudioObject> object)
{
	this->object = std::move(object);
	event = {};
	updatePlaybackObject();
}

void AudioPlaybackPanel::setAudioEvent(std::shared_ptr<const AudioEvent> event)
{
	this->event = std::move(event);
	object = {};
	needsObjectUpdate = false;
	updatePlaybackObject();
}

void AudioPlaybackPanel::onPlay()
{
	needsIconUpdate = true;
	if (isPlaying()) {
		pause();
	} else {
		play();
	}
}

void AudioPlaybackPanel::play()
{
	if (isPlaying()) {
		pause();
	}

	if (!emitter) {
		emitter = api.audio->createEmitter(AudioPosition());
		api.audio->setListener(AudioListenerData(Vector3f()));
	}

	if (object) {
		audioHandle = api.audio->play(playbackObject, emitter);
	} else if (event) {
		audioHandle = api.audio->postEvent(*event, emitter);
	}
}

void AudioPlaybackPanel::pause()
{
	if (!isPlaying()) {
		return;
	}

	audioHandle->stop();
	audioHandle = {};
}

bool AudioPlaybackPanel::isPlaying() const
{
	return static_cast<bool>(audioHandle);
}

void AudioPlaybackPanel::updatePlaybackObject()
{
	if (object) {
		if (playbackObject) {
			// Update existing
			auto newObject = AudioObject(*object);
			newObject.loadDependencies(project.getGameResources());

			// Note that this is only safe because it waits for the callback to complete
			api.audio->runOnAudioThread([&] () {
				playbackObject->reload(std::move(newObject));
			}).wait();
		} else {
			// Create new
			playbackObject = std::make_shared<AudioObject>(*object);
			playbackObject->loadDependencies(project.getGameResources());
		}
	} else {
		playbackObject = {};
	}
}
