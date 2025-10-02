#include "audio_preview_button.h"

using namespace Halley;

AudioPreviewButton::AudioPreviewButton(UIFactory& factory, Resources& resources, const HalleyAPI& api, AssetType type, String assetId)
	: UIWidget("audio_preview_button", {}, UISizer())
	, factory(factory)
	, resources(resources)
	, api(api)
	, type(type)
	, assetId(std::move(assetId))
{
	factory.loadUI(*this, "halley/audio_editor/audio_preview_button");
}

void AudioPreviewButton::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "play", [=] (const UIEvent& event) {
		onPlay();
	});
}

void AudioPreviewButton::update(Time t, bool moved)
{
	bool playing = isPlaying();

	if (playing) {
		// Someone else is playing
		if (getRoot()->getSharedData<AudioPreviewUISharedData>("audio_preview_button")->lastActive != this) {
			stop();
			playing = false;
		}
	}

	if (playing != buttonShowsPlaying) {
		const char* image = isPlaying() ? "halley_ui/icon_pause.png" : "halley_ui/icon_play.png";
		getWidgetAs<UIButton>("play")->setIcon(Sprite().setImage(factory.getResources(), image));
		buttonShowsPlaying = playing;
	}
}

void AudioPreviewButton::onPlay()
{
	if (isPlaying()) {
		stop();
	} else {
		play();
	}
}

void AudioPreviewButton::play()
{
	if (isPlaying()) {
		stop();
	}

	if (!emitter) {
		emitter = api.audio->createEmitter({});
	}

	api.audio->resetBuses();

	if (type == AssetType::AudioEvent) {
		audioHandle = api.audio->postEvent(assetId, emitter);
	} else if (type == AssetType::AudioObject) {
		auto object = resources.get<AudioObject>(assetId);
		audioHandle = api.audio->play(std::move(object), emitter);
	} else if (type == AssetType::AudioClip) {
		auto clip = resources.get<AudioClip>(assetId);
		audioHandle = api.audio->play(std::move(clip), emitter);
	}

	if (isPlaying()) {
		getRoot()->getSharedData<AudioPreviewUISharedData>("audio_preview_button")->lastActive = this;
	}
}

void AudioPreviewButton::stop()
{
	if (audioHandle) {
		audioHandle->stop();
	}

	audioHandle = {};
	emitter = {};
}

bool AudioPreviewButton::isPlaying()
{
	return audioHandle && audioHandle->isPlaying();
}
