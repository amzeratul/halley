#include "behaviours/ui_transition_slide_behaviour.h"
#include "ui_widget.h"
using namespace Halley;

UITransitionSlideBehaviour::UITransitionSlideBehaviour(Time length, UIAnchor base, UIAnchor fadeIn, UIAnchor fadeOut)
	: length(length)
	, time(0.0)
	, mode(Mode::FadeIn)
	, base(std::move(base))
	, fadeIn(std::move(fadeIn))
	, fadeOut(std::move(fadeOut))
{
}

void UITransitionSlideBehaviour::init()
{
	getWidget()->setEnabled(false);
	getWidget()->setAnchor(fadeIn);
}

void UITransitionSlideBehaviour::deInit()
{
}

void UITransitionSlideBehaviour::update(Time dt)
{
	if (mode != Mode::Normal) {
		time += dt;
		
		const float x = clamp(mode == Mode::FadeIn ? float(time / length) : 1.0f - float(time / length), 0.0f, 1.0f);
		constexpr float pi = 3.141592653592f;
		//const float t = smoothCos(x);
		//const float t = ::sin(1.7f * x * x) / ::sin(1.7f);
		//const float t = (1.0f - x) * ::sin(2.5f * 3.1415926535 * ::pow(x, 5.0f)) * 1.2 + x;
		const float t = float(pow(1.0f - x, 3.0f) * sin(2.5f * pi * pow(x, 3.0f)) * 4.0f + (1.0f - pow(1.0f - x, 3.0f)) * sin(x * pi * 0.5f));

		if (mode == Mode::FadeIn) {
			if (time >= length) {
				mode = Mode::Normal;
				getWidget()->setEnabled(true);
			}
			getWidget()->setAnchor(lerp(fadeIn, base, t));
		} else if (mode == Mode::FadeOut) {
			getWidget()->setAnchor(lerp(base, fadeOut, 1.0f - t));
			if (time >= length) {
				getWidget()->destroy();
			}
		}
	}
}

bool UITransitionSlideBehaviour::isAlive() const
{
	return true;
}

bool UITransitionSlideBehaviour::onParentDestroyed()
{
	if (mode == Mode::FadeOut) {
		return time >= length;
	} else {
		mode = Mode::FadeOut;
		time = 0;
		getWidget()->setEnabled(false);
		return false;
	}
}
