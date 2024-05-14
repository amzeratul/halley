#include "halley/diagnostics/audio_view.h"

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
}

void AudioView::paint(Painter& painter)
{
	if (!active) {
		return;
	}

	headerText
		.setPosition(Vector2f(10, 10))
		.setOffset(Vector2f())
		.setOutline(1.0f)
		.setText("Audio View")
		.draw(painter);
}
