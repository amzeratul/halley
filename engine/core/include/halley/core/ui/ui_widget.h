#pragma once
#include "ui_sizer.h"
#include "ui_root.h"
#include "ui_painter.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class UIEvent;
	class UIValidator;

	class UIWidget : public IUIElement, public UIParent, public IUISizer {
		friend class UIParent;
		friend class UIRoot;

	public:
		UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget();

		void doDraw(UIPainter& painter) const;
		void doUpdate(Time t, UIInputType inputType, InputDevice& inputDevice);

		Vector2f computeMinimumSize() const override;
		void setRect(Rect4f rect) override;

		void layout(bool shrink = false);
		void centerAt(Vector2f position);
		
		virtual Maybe<UISizer>& tryGetSizer();
		virtual UISizer& getSizer();

		void add(std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void add(std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;

		virtual bool isFocusable() const;
		virtual bool isFocusLocked() const;
		bool isMouseOver() const;
		bool isFocused() const;

		String getId() const;

		Vector2f getPosition() const;
		Vector2f getSize() const;
		Vector2f getMinimumSize() const;
		Vector4f getInnerBorder() const;

		void setPosition(Vector2f pos);
		void setMinSize(Vector2f size);

		void setFocused(bool focused);
		void setMouseOver(bool mouseOver);
		virtual void pressMouse(Vector2f mousePos, int button);
		virtual void releaseMouse(Vector2f mousePos, int button);
		virtual Rect4f getMouseRect() const;

		bool isShown() const override;
		void setShown(bool shown);
		bool isEnabled() const;
		void setEnabled(bool enabled);
		bool isModal() const;
		void setModal(bool modal);
		bool isMouseBlocker() const;
		void setMouseBlocker(bool blocker);

		bool isAlive() const;
		void destroy();

		void setEventHandler(std::shared_ptr<UIEventHandler> handler);
		UIEventHandler& getEventHandler();
		
		virtual void setInputType(UIInputType uiInput);
		void setOnlyEnabledWithInputs(const std::vector<UIInputType>& inputs);
		const std::vector<UIInputType>& getOnlyEnabledWithInput() const;

		void setValidator(std::shared_ptr<UIValidator> validator);
		std::shared_ptr<UIValidator> getValidator() const;

	protected:
		virtual void draw(UIPainter& painter) const;
		virtual void update(Time t, bool moved);
		virtual void updateInputDevice(InputDevice& device);

		virtual void onFocus();
		virtual void onFocusLost();
		UIRoot* getRoot() override;

		virtual void onEnabledChanged();

		void sendEvent(UIEvent&& event) const override;

		void playSound(const std::shared_ptr<const AudioClip>& clip);

	private:
		void setWidgetRect(Rect4f rect);
		void setParent(UIParent& parent);

		UIParent* parent = nullptr;
		String id;
		std::vector<UIInputType> onlyEnabledWithInputs;

		Vector2f position;
		Vector2f size;
		Vector2f minSize;

		Vector4f innerBorder;
		Maybe<UISizer> sizer;

		std::shared_ptr<UIEventHandler> eventHandler;
		std::shared_ptr<UIValidator> validator;

		bool shown = true;
		bool enabled = true;
		bool alive = true;
		bool focused = false;
		bool mouseOver = false;
		bool positionUpdated = false;
		bool modal = true;
		bool mouseBlocker = true;
	};
}
