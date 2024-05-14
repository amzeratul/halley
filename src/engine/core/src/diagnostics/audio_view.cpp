#include "halley/diagnostics/audio_view.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/painter.h"

using namespace Halley;

AudioView::AudioView(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, boxBg(Sprite().setImage(resources, "halley/box_2px_outline.png"))
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
}

AudioView::~AudioView()
{
	if (listenerRegistered) {
		listenerRegistered = false;
		api.audio->setDebugListener(nullptr);
	}
}

void AudioView::update(Time t)
{
	if (active != listenerRegistered) {
		if (active) {
			api.audio->setDebugListener(this);
			listenerRegistered = true;
		} else {
			api.audio->setDebugListener(nullptr);
			listenerRegistered = false;
		}
	}
}

void AudioView::paint(Painter& painter)
{
	if (!active) {
		return;
	}

	const auto rect = Rect4f(painter.getViewPort());
	whitebox.clone().setPosition(Vector2f(0, 0)).scaleTo(Vector2f(rect.getSize())).setColour(Colour4f(0, 0, 0, 0.5f)).draw(painter);

	headerText
		.setPosition(Vector2f(10, 10))
		.setOffset(Vector2f())
		.setOutline(1.0f)
		.setText("Audio View")
		.draw(painter);
}

void AudioView::onAudioDebugData(AudioDebugData data)
{
	lastData = data;
}
