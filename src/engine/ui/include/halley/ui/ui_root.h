#pragma once
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "halley/core/input/input_virtual.h"
#include "ui_parent.h"
#include "ui_input.h"
#include "halley/core/api/audio_api.h"
#include "halley/core/game/core.h"

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
	
	class UIRoot final : public UIParent {
	public:
		explicit UIRoot(AudioAPI* audio, Rect4f rect = {});

		UIRoot* getRoot() override;
		const UIRoot* getRoot() const override;
		const String& getId() const override;

		void setRect(Rect4f rect, Vector2f overscan = Vector2f());
		Rect4f getRect() const override;

		void update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual);
		void draw(SpritePainter& painter, int mask, int layer);
		void render(RenderContext& rc);

		void mouseOverNext(bool forward = true);
		void runLayout();
		
		std::optional<AudioHandle> playSound(const String& eventName);
		void sendEvent(UIEvent&& event) const override;

		bool hasModalUI() const;
		bool isMouseOverUI() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouse() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouseIncludingDisabled() const;
		void setFocus(const std::shared_ptr<UIWidget>& focus);
		void focusNext(bool reverse);

		UIWidget* getCurrentFocus() const;

		void setUIMouseRemapping(std::function<Vector2f(Vector2f)> remapFunction);
		void unsetUIMouseRemapping();

		std::vector<std::shared_ptr<UIWidget>> collectWidgets();

		void onChildAdded(UIWidget& child) override;
		
	private:
		String id;
		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;
		std::shared_ptr<InputDevice> dummyInput;
		Rect4f uiRect;
		Vector2f overscan;

		AudioAPI* audio;
		bool anyMouseButtonHeld = false;

		std::function<Vector2f(Vector2f)> mouseRemap;

		void updateMouse(spInputDevice mouse);
		void updateInputTree(const spInputDevice& input, UIWidget& c, std::vector<UIWidget*>& inputTargets, UIInput::Priority& bestPriority, bool accepting);
		void updateInput(spInputDevice input);

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos, bool includeDisabled = false) const;
		std::shared_ptr<UIWidget> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos, bool includeDisabled = false) const;
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);
	};
}
