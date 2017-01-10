#pragma once
#include "halley/time/halleytime.h"
#include <vector>
#include "halley/maths/vector2.h"

namespace Halley {
	class TextRenderer;
	class Sprite;
	class SpritePainter;
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent() {}

		virtual UIRoot& getRoot() = 0;

		void addChild(std::shared_ptr<UIWidget> widget);
		void removeChild(UIWidget& widget);

		std::vector<std::shared_ptr<UIWidget>>& getChildren();
		const std::vector<std::shared_ptr<UIWidget>>& getChildren() const;

	private:
		std::vector<std::shared_ptr<UIWidget>> children;
	};

	class UIPainter {
	public:
		UIPainter(SpritePainter& painter, int mask, int layer);

		void draw(const Sprite& sprite);
		void draw(const TextRenderer& sprite);

	private:
		SpritePainter& painter;
		int mask;
		int layer;
		int n;
	};
	
	class UIRoot : public UIParent {
	public:
		UIRoot& getRoot() override;

		void addWidget(UIWidget& widget);
		void removeWidget(UIWidget& widget);

		void update(Time t, Vector2f mousePos, bool leftClick, bool rightClick);
		void draw(SpritePainter& painter, int mask, int layer);

	private:
		std::vector<UIWidget*> widgets;
	};
}
