#pragma once
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "halley/core/input/input_virtual.h"
#include "ui_parent.h"
#include "ui_input.h"

namespace Halley {
	class SpritePainter;
	class AudioAPI;
	class AudioClip;

	enum class UIInputType {
		Undefined,
		Mouse,
		Keyboard,
		Gamepad
	};
	
	class UIRoot : public UIParent {
	public:
		explicit UIRoot(AudioAPI* audio, Rect4f rect = {});

		UIRoot* getRoot() override;
		const String& getId() const override;

		void setRect(Rect4f rect);
		Rect4f getRect() const override;

		void update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual);
		void draw(SpritePainter& painter, int mask, int layer);
		void mouseOverNext(bool forward = true);
		void runLayout();
		
		void playSound(const String& eventName);
		void sendEvent(UIEvent&& event) const override;

		bool hasModalUI() const;
		bool isMouseOverUI() const;
		void setFocus(std::shared_ptr<UIWidget> focus);

		UIWidget* getCurrentFocus() const;

		std::vector<std::shared_ptr<UIWidget>> collectWidgets();

	private:
		String id;
		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;
		std::shared_ptr<InputDevice> dummyInput;
		Rect4f uiRect;

		AudioAPI* audio;
		bool mouseHeld = false;

		void updateMouse(spInputDevice mouse);
		void updateInputTree(const spInputDevice& input, UIWidget& c, std::vector<UIWidget*>& inputTargets, UIInput::Priority& bestPriority, bool accepting);
		void updateInput(spInputDevice input);

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos);
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos);
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);
	};
}
