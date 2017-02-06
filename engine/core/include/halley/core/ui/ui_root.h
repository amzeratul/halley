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

	enum class UIInputType {
		Undefined,
		Mouse,
		Keyboard,
		Gamepad
	};

	class UIParent {
	public:
		virtual ~UIParent() {}

		virtual UIRoot* getRoot() = 0;
		virtual void sendEvent(UIEvent&& event) const = 0;

		void addChild(std::shared_ptr<UIWidget> widget);
		void removeChild(UIWidget& widget);

		std::vector<std::shared_ptr<UIWidget>>& getChildren();
		const std::vector<std::shared_ptr<UIWidget>>& getChildren() const;
		
	protected:
		bool topChildChanged = false;

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

		void update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual, Vector2f uiOffset = {});
		void draw(SpritePainter& painter, int mask, int layer);
		void mouseOverNext(bool forward = true);
		void runLayout();
		
		void playSound(const std::shared_ptr<const AudioClip>& clip);
		void sendEvent(UIEvent&& event) const override;

		bool hasModalUI() const;

		std::vector<std::shared_ptr<UIWidget>> collectWidgets();

	private:
		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;

		AudioAPI* audio;
		bool mouseHeld = false;

		UIInputType lastInputType = UIInputType::Undefined;

		void updateMouse(spInputDevice mouse, Vector2f uiOffset);
		void updateManual(spInputDevice manual);

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos);
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos);
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void setFocus(std::shared_ptr<UIWidget> focus);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);
		void convertToInputType(UIInputType activeInput);
	};
}
