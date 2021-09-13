#pragma once
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"
#include "ui_event.h"
#include "ui_parent.h"
#include "ui_input.h"

namespace Halley {
	class UIStyle;
	class RenderContext;
	class HalleyAPI;
	class SpritePainter;
	class AudioAPI;
	class AudioClip;
	class TextInputCapture;
	class InputDevice;
	class IAudioHandle;
	class UIToolTip;

	enum class UIInputType {
		Undefined,
		Mouse,
		Keyboard,
		Gamepad
	};
	
	class UIRoot final : public UIParent {
	public:
		explicit UIRoot(const HalleyAPI& api, Rect4f rect = {});
		~UIRoot();

		UIRoot* getRoot() override;
		const UIRoot* getRoot() const override;
		const String& getId() const override;

		void setRect(Rect4f rect, Vector2f overscan = Vector2f());
		Rect4f getRect() const override;

		void update(Time t, UIInputType activeInputType, std::shared_ptr<InputDevice> mouse, std::shared_ptr<InputDevice> manual);
		void draw(SpritePainter& painter, int mask, int layer);
		void render(RenderContext& rc);

		void mouseOverNext(bool forward = true);
		void runLayout();
		
		std::optional<std::shared_ptr<IAudioHandle>> playSound(const String& eventName);
		void sendEvent(UIEvent event) const override;

		bool hasModalUI() const;
		bool isMouseOverUI() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouse() const;
		std::shared_ptr<UIWidget> getWidgetUnderMouseIncludingDisabled() const;
		void setFocus(const std::shared_ptr<UIWidget>& newFocus);
		void focusNext(bool reverse);
		void onWidgetRemoved(const UIWidget& widget);

		UIWidget* getCurrentFocus() const;

		void setUIMouseRemapping(std::function<Vector2f(Vector2f)> remapFunction);
		void unsetUIMouseRemapping();

		std::vector<std::shared_ptr<UIWidget>> collectWidgets();

		void onChildAdded(UIWidget& child) override;
		
		void registerKeyPressListener(std::shared_ptr<UIWidget> widget, int priority = 0);
		void removeKeyPressListener(const UIWidget& widget);
		void setUnhandledKeyPressListener(std::function<bool(KeyboardKeyPress)> handler);

		void makeToolTip(const UIStyle& style);

		Vector2f getLastMousePos() const;

	private:
		String id;
		std::shared_ptr<InputKeyboard> keyboard;
		AudioAPI* audio = nullptr;
		Rect4f uiRect;

		std::weak_ptr<UIWidget> currentMouseOver;
		std::weak_ptr<UIWidget> mouseExclusive; // A widget that's taking exclusive control of mouse
		std::weak_ptr<UIWidget> currentFocus;
		Vector2f lastMousePos;
		std::shared_ptr<InputDevice> dummyInput;
		Vector2f overscan;

		bool anyMouseButtonHeld = false;

		std::function<Vector2f(Vector2f)> mouseRemap;
		std::unique_ptr<TextInputCapture> textCapture;
		std::vector<std::pair<std::weak_ptr<UIWidget>, int>> keyPressListeners;
		std::function<bool(KeyboardKeyPress)> unhandledKeyPressListener;

		std::shared_ptr<UIToolTip> toolTip;

		void updateMouse(const std::shared_ptr<InputDevice>& mouse, KeyMods keyMods);
		void updateGamepadInputTree(const std::shared_ptr<InputDevice>& input, UIWidget& c, std::vector<UIWidget*>& inputTargets, UIGamepadInput::Priority& bestPriority, bool accepting);
		void updateGamepadInput(const std::shared_ptr<InputDevice>& input);

		void updateKeyboardInput();
		void sendKeyPress(KeyboardKeyPress key);
		void onUnhandledKeyPress(KeyboardKeyPress key);
		void receiveKeyPress(KeyboardKeyPress key) override;
		KeyMods getKeyMods();

		std::shared_ptr<UIWidget> getWidgetUnderMouse(Vector2f mousePos, bool includeDisabled = false) const;
		std::pair<std::shared_ptr<UIWidget>, int> getWidgetUnderMouse(const std::shared_ptr<UIWidget>& curWidget, Vector2f mousePos, bool includeDisabled = false, int childLayerAdjustment = 0) const;
		void updateMouseOver(const std::shared_ptr<UIWidget>& underMouse);
		void collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output);

		void focusWidget(UIWidget& widget);
		void unfocusWidget(UIWidget& widget);
	};
}
