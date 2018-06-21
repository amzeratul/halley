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

		virtual bool isAlive() const;
		UIWidget* getWidget() const;

	private:
		friend class UIWidget;

		UIWidget* widget = nullptr;

		void doInit(UIWidget& widget);
		void doDeInit();
    };
}
