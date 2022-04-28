#include "audio_fade_editor.h"

#include "halley/audio/audio_fade.h"
#include "halley/ui/ui_factory.h"
using namespace Halley;

AudioFadeEditor::AudioFadeEditor(UIFactory& factory, AudioFade& fade, std::function<void()> onModifiedCallback)
	: UIWidget("audio_fade_editor", Vector2f(), UISizer())
	, factory(factory)
	, fade(fade)
	, onModifiedCallback(std::move(onModifiedCallback))
{
	factory.loadUI(*this, "halley/audio_editor/audio_fade_editor");
}

void AudioFadeEditor::onMakeUI()
{
	auto updateFadeType = [this](AudioFadeCurve curve)
	{
		getWidget("fadeLenOptions")->setActive(curve != AudioFadeCurve::None);
	};

	bindData("fadeType", toString(fade.getCurve()), [=] (String value)
	{
		auto curve = fromString<AudioFadeCurve>(value);
		fade.setCurve(curve);
		if (onModifiedCallback) {
			onModifiedCallback();
		}
		updateFadeType(curve);
	});
	updateFadeType(fade.getCurve());

	bindData("fadeLength", fade.getLength(), [=] (float value)
	{
		fade.setLength(value);
		if (onModifiedCallback) {
			onModifiedCallback();
		}
	});	
}
