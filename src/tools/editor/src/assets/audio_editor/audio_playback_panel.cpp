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
}

void AudioPlaybackPanel::onActiveChanged(bool active)
{
	if (!active) {
		pause();
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
	if (!playbackObject && object) {
		playbackObject = std::make_shared<AudioObject>(*object);
		playbackObject->loadDependencies(project.getGameResources());
	}

	if (!object && !playbackObject) {
		playbackObject = {};
	}
}
