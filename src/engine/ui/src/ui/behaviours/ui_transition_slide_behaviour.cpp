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
		const float t = smoothCos(clamp(float(time / length), 0.0f, 1.0f));

		if (mode == Mode::FadeIn) {
			if (time >= length) {
				mode = Mode::Normal;
				getWidget()->setEnabled(true);
			}
			getWidget()->setAnchor(lerp(fadeIn, base, t));
		} else if (mode == Mode::FadeOut) {
			getWidget()->setAnchor(lerp(base, fadeOut, t));
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
