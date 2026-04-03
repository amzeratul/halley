#pragma once

#include "../ui_behaviour.h"
#include "halley/maths/interpolation_curve.h"

namespace Halley {
	class UIAnchor;

	class UIFadeBehaviour final : public UIBehaviour {
	public:
		UIFadeBehaviour(Time delay, Time length, InterpolationCurve curve, bool reversed=false);

		void init() override;
		void update(Time time) override;
		bool isAlive() const override;

		void restart() override;

	private:
		Time delay;
		Time length;
		InterpolationCurve curve;

		Time curTime = 0;

		void applyFade();
	};
}
