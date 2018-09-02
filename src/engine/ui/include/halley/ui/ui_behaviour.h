#pragma once
#include "halley/time/halleytime.h"

namespace Halley {
	class UIWidget;

	class UIBehaviour
    {
    public:
		virtual ~UIBehaviour();

	    virtual void init();
		virtual void deInit();
		virtual void update(Time time);

		virtual bool onParentDestroyed(); // Return true if OK, false to abort destruction

		virtual bool isAlive() const;
		UIWidget* getWidget() const;

		virtual void setReversed(bool reversed);
		virtual bool isReversed() const;

	private:
		friend class UIWidget;

		UIWidget* widget = nullptr;
		bool reversed = false;

		void doInit(UIWidget& widget);
		void doDeInit();
    };
}
