#pragma once
#include "halley/time/halleytime.h"
#include <vector>
#include "halley/maths/vector2.h"

namespace Halley {
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent() {}

		virtual UIRoot& getRoot() = 0;

		void addChild(UIWidget& widget);
		void removeChild(UIWidget& widget);

		std::vector<UIWidget*>& getChildren();

	private:
		std::vector<UIWidget*> children;
	};
	
	class UIRoot : public UIParent {
	public:
		UIRoot& getRoot() override;

		void addWidget(UIWidget& widget);
		void removeWidget(UIWidget& widget);

		void update(Time t, Vector2f mousePos, bool leftClick, bool rightClick);

	private:
		std::vector<UIWidget*> widgets;
	};
}
