#include "audio_playback_panel.h"

using namespace Halley;

AudioPlaybackPanel::AudioPlaybackPanel(UIFactory& factory, const HalleyAPI& api)
	: UIWidget("audio_playback", {}, UISizer())
	, factory(factory)
	, api(api)
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

void AudioPlaybackPanel::setAudioObject(std::shared_ptr<const AudioObject> object)
{
	this->object = std::move(object);
	event = {};
}

void AudioPlaybackPanel::setAudioEvent(std::shared_ptr<const AudioEvent> event)
{
	this->event = std::move(event);
	object = {};
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
	if (!emitter) {
		emitter = api.audio->createEmitter(AudioPosition());
		api.audio->setListener(AudioListenerData(Vector3f()));
	}

	if (object) {
		audioHandle = api.audio->play(object, emitter);
	} else if (event) {
		audioHandle = api.audio->postEvent(*event, emitter);
	}
}

void AudioPlaybackPanel::pause()
{
	audioHandle->stop();
	audioHandle = {};
}

bool AudioPlaybackPanel::isPlaying() const
{
	return static_cast<bool>(audioHandle);
}
