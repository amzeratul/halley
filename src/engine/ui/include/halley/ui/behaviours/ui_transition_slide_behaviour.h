#pragma once

#include "../ui_behaviour.h"
#include "../ui_anchor.h"

namespace Halley {
	class UIAnchor;

	class UITransitionSlideBehaviour : public UIBehaviour {
		enum class Mode {
			FadeIn,
			Normal,
			FadeOut
		};

    public:
		using TransitionCurve = std::function<float(float)>;

		UITransitionSlideBehaviour(Time length, UIAnchor base, UIAnchor startPos, UIAnchor endPos, TransitionCurve curve);

	    void init() override;
	    void deInit() override;
	    void update(Time time) override;
	    bool isAlive() const override;
	    bool onParentDestroyed() override;

    private:
		Time length;
		Time time;
		Mode mode;
		TransitionCurve curve;

		UIAnchor base;
		UIAnchor startPos;
		UIAnchor endPos;
    };
}
