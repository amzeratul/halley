#include "halley/ui/behaviours/ui_slide_behaviour.h"
#include "halley/ui/ui_widget.h"
using namespace Halley;

UISlideBehaviour::UISlideBehaviour(Time delay, Time length, Vector2f offset, InterpolationCurve curve)
	: delay(delay)
	, length(length)
	, offset(offset)
	, curve(std::move(curve))
{
}

void UISlideBehaviour::init()
{
	getWidget()->setPositionOffset(offset);
}

void UISlideBehaviour::update(Time time)
{
	curTime += time;
	const auto alpha = static_cast<float>(clamp((curTime - delay) / std::max(length, 0.01), 0.0, 1.0));
	const auto t = curve.evaluate(isReversed() ? 1.0f - alpha : alpha);
	getWidget()->setPositionOffset(lerp(offset, Vector2f(), t));
}

bool UISlideBehaviour::isAlive() const
{
	return curTime <= delay + length;
}

void UISlideBehaviour::restart()
{
	curTime = 0;
}
