#include "halley/ui/behaviours/ui_fade_behaviour.h"
#include "halley/ui/ui_widget.h"
using namespace Halley;

UIFadeBehaviour::UIFadeBehaviour(Time delay, Time length, InterpolationCurve curve, bool reversed)
	: delay(delay)
	, length(length)
	, curve(std::move(curve))
{
	setReversed(reversed);
}

void UIFadeBehaviour::init()
{
	restart();
}

void UIFadeBehaviour::update(Time time)
{
	curTime += time;
	applyFade();
}

bool UIFadeBehaviour::isAlive() const
{
	return curTime <= delay + length;
}

void UIFadeBehaviour::restart()
{
	curTime = 0;
	applyFade();
}

void UIFadeBehaviour::applyFade()
{
	if (auto w = getWidget()) {
		const auto alpha = static_cast<float>(clamp((curTime - delay) / std::max(length, 0.01), 0.0, 1.0));
		const auto t = curve.evaluate(isReversed() ? 1.0f - alpha : alpha);
		w->setDynamicValue("alpha", ConfigNode(t));
	}
}
