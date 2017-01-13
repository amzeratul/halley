#pragma once
#include "halley/time/halleytime.h"
#include <vector>
#include "halley/maths/vector2.h"
#include "ui_event.h"

namespace Halley {
	class AudioAPI;
	class AudioClip;
	class TextRenderer;
	class Sprite;
	class SpritePainter;
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent() {}

		virtual UIRoot& getRoot() = 0;
		virtual void sendEvent(UIEvent&& event) const = 0;

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

		explicit UIRoot(AudioAPI* audio);

		void addWidget(UIWidget& widget);
		void removeWidget(UIWidget& widget);

		void update(Time t, Vector2f mousePos, bool mousePressed, bool mouseReleased);
		void draw(SpritePainter& painter, int mask, int layer);
		
		void playSound(const std::shared_ptr<const AudioClip>& clip);

		void sendEvent(UIEvent&& event) const override;

	private:
		std::vector<UIWidget*> widgets;
		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> currentFocus;

		AudioAPI* audio;
		bool mouseHeld = false;

		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos);
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void setFocus(std::shared_ptr<UIWidget> focus);
	};
}
