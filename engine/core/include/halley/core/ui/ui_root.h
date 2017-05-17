#pragma once
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "halley/core/input/input_virtual.h"
#include "ui_parent.h"

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
		std::shared_ptr<InputDevice> dummyInput;

		AudioAPI* audio;
		bool mouseHeld = false;

		void updateMouse(spInputDevice mouse, Vector2f uiOffset);
		void updateTabbing(spInputDevice manual);

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos);
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos);
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void setFocus(std::shared_ptr<UIWidget> focus);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);
	};
}
