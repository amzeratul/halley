#pragma once
#include "ui_sizer.h"
#include "ui_root.h"
#include "ui_painter.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"
#include "ui_input.h"
#include "ui_style.h"

namespace Halley {
	class UIEvent;
	class UIValidator;
	class UIDataBind;

	class UIWidget : public IUIElement, public UIParent, public IUISizer, public std::enable_shared_from_this<UIWidget> {
		friend class UIParent;
		friend class UIRoot;

	public:
		UIWidget(String id = "", Vector2f minSize = {}, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget();

		void doDraw(UIPainter& painter) const;
		void doUpdate(bool full, Time t, UIInputType inputType, JoystickType joystickType);

		Vector2f getLayoutMinimumSize(bool force) const override;
		void setRect(Rect4f rect) override;

		void layout();
		virtual void alignAt(Vector2f position, Vector2f alignment, Maybe<Rect4f> bounds = {});
		void centerAt(Vector2f position, Maybe<Rect4f> bounds = {});
		
		virtual Maybe<UISizer>& tryGetSizer();
		virtual UISizer& getSizer();

		void add(std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void add(std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;

		virtual bool canInteractWithMouse() const;
		virtual bool isFocusLocked() const;
		bool isMouseOver() const;
		bool isFocused() const;

		void setId(const String& id);
		const String& getId() const;

		Vector2f getPosition() const;
		virtual Vector2f getLayoutOriginPosition() const;
		Vector2f getSize() const;
		Vector2f getMinimumSize() const;
		Vector4f getInnerBorder() const;

		void setPosition(Vector2f pos);
		void setMinSize(Vector2f size);
		void setInnerBorder(Vector4f border);

		void setFocused(bool focused);
		void setMouseOver(bool mouseOver);
		virtual void pressMouse(Vector2f mousePos, int button);
		virtual void releaseMouse(Vector2f mousePos, int button);
		virtual void onMouseOver(Vector2f mousePos);
		virtual Rect4f getMouseRect() const;

		bool isActive() const override;
		void setActive(bool active);
		bool isEnabled() const;
		void setEnabled(bool enabled);
		bool isModal() const;
		void setModal(bool modal);
		bool isMouseBlocker() const;
		void setMouseBlocker(bool blocker);
		bool shrinksOnLayout() const;
		void setShrinkOnLayout(bool shrink);

		bool isAlive() const;
		void destroy();

		void setEventHandler(std::shared_ptr<UIEventHandler> handler);
		UIEventHandler& getEventHandler();
		void setHandle(UIEventType type, UIEventCallback handler);
		void setHandle(UIEventType type, String id, UIEventCallback handler);
		
		virtual void setInputType(UIInputType uiInput);
		virtual void setJoystickType(JoystickType joystickType);
		void setOnlyEnabledWithInputs(const std::vector<UIInputType>& inputs);
		const std::vector<UIInputType>& getOnlyEnabledWithInput() const;
		virtual void setInputButtons(const UIInputButtons& buttons);

		void setValidator(std::shared_ptr<UIValidator> validator);
		std::shared_ptr<UIValidator> getValidator() const;
		void setDataBind(std::shared_ptr<UIDataBind> dataBind);
		std::shared_ptr<UIDataBind> getDataBind() const;
		virtual void readFromDataBind();
		
		bool isDescendentOf(const UIWidget& ancestor) const override;
		void setMouseClip(Maybe<Rect4f> mouseClip);

		virtual void onManualControlCycleValue(int delta);
		virtual void onManualControlActivate();
		
		UIInput::Priority getInputPriority() const;

		void setChildLayerAdjustment(int delta);
		int getChildLayerAdjustment() const;

		void sendEvent(UIEvent&& event) const override;
		void sendEventDown(const UIEvent& event) const;
		void forceAddChildren(UIInputType inputType);

	protected:
		virtual void draw(UIPainter& painter) const;
		virtual void drawAfterChildren(UIPainter& painter) const;
		virtual void drawChildren(UIPainter& painter) const;
		virtual void update(Time t, bool moved);

		virtual void onFocus();
		virtual void onFocusLost();
		virtual void onLayout();
		UIRoot* getRoot() override;

		void notifyDataBind(int data) const;
		void notifyDataBind(float data) const;
		void notifyDataBind(const String& data) const;

		void shrink();
		void forceLayout();
		UIInputType getLastInputType() const;

		virtual void onInput(const UIInputResults& input);
		virtual void updateInputDevice(const InputDevice& inputDevice);

		virtual void onEnabledChanged();

		void playSound(const std::shared_ptr<const AudioClip>& clip);
		virtual void checkActive();

		UIInputType lastInputType = UIInputType::Undefined;

	private:
		void setParent(UIParent* parent);
		UIParent* getParent() const;

		void setWidgetRect(Rect4f rect);

		UIParent* parent = nullptr;
		String id;
		std::vector<UIInputType> onlyEnabledWithInputs;
		std::unique_ptr<UIInputButtons> inputButtons;
		UIInputResults inputResults;

		Vector2f position;
		Vector2f size;
		Vector2f minSize;

		Vector4f innerBorder;
		Maybe<UISizer> sizer;
		Maybe<Rect4f> mouseClip;

		std::shared_ptr<UIEventHandler> eventHandler;
		std::shared_ptr<UIValidator> validator;
		std::shared_ptr<UIDataBind> dataBind;

		int childLayerAdjustment = 0;

		bool active = true;
		bool enabled = true;
		bool alive = true;
		bool focused = false;
		bool mouseOver = false;
		bool positionUpdated = false;
		bool modal = true;
		bool mouseBlocker = true;
		bool shrinkOnLayout = false;
	};
}
