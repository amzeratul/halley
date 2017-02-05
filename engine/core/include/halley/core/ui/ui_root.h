#pragma once
#include "halley/time/halleytime.h"
#include <vector>
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "halley/core/input/input_virtual.h"

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

		virtual UIRoot* getRoot() = 0;
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
		UIRoot* getRoot() override;

		explicit UIRoot(AudioAPI* audio);

		void update(Time t, spInputDevice mouse, spInputDevice manual, Vector2f uiOffset = {});
		void draw(SpritePainter& painter, int mask, int layer);
		
		void playSound(const std::shared_ptr<const AudioClip>& clip);

		void sendEvent(UIEvent&& event) const override;

		bool hasModalUI() const;

	private:
		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;

		AudioAPI* audio;
		bool mouseHeld = false;

		void updateMouse(spInputDevice mouse, Vector2f uiOffset);
		void updateManual(spInputDevice manual);
		void mouseOverNext(bool forward = true);

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos);
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos);
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void setFocus(std::shared_ptr<UIWidget> focus);

		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);
	};
}
