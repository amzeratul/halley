#pragma once
#include "ui_root.h"
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
		virtual void onAddedToRoot(UIRoot& root);

		virtual bool onParentDestroyed(); // Return true if OK, false to abort destruction
		virtual void onParentAboutToDraw();

		virtual bool isAlive() const;
		UIWidget* getWidget() const;

		virtual void setReversed(bool reversed);
		virtual bool isReversed() const;

		void setInitial(bool initial);
		bool isInitial() const;
		virtual void restart();

	private:
		friend class UIWidget;

		UIWidget* widget = nullptr;
		bool reversed = false;
		bool initial = false;

		void doInit(UIWidget& widget);
		void doDeInit();
    };
}
