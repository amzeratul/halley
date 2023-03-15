#pragma once

#include "../ui_behaviour.h"
#include "../ui_anchor.h"

namespace Halley {
	class UIAnchor;

	class UITransitionSlideBehaviour final : public UIBehaviour {
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

		void updateAnchors(UIAnchor newBase, UIAnchor newStartPos, UIAnchor newEndPos);

		bool isFinished() const;

    private:
		Time length;
		Time time;
		Mode mode;
		TransitionCurve curve;

		UIAnchor base;
		UIAnchor startPos;
		UIAnchor endPos;

		bool finished;
    };
}
