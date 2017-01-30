#pragma once
#include "ui_sizer.h"
#include "ui_root.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class UIEvent;
	class Painter;

	class UIWidget : public IUIElement, public UIParent, public IUISizer {
		friend class UIParent;

	public:
		UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget();

		void doDraw(UIPainter& painter) const;
		void doUpdate(Time t);

		Vector2f computeMinimumSize() const override;
		void setRect(Rect4f rect) override;

		void layout();
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
		virtual void pressMouse(int button);
		virtual void releaseMouse(int button);

		void destroy();

		std::shared_ptr<UIWidget> getWidget(const String& id);

		void setEventHandler(std::shared_ptr<UIEventHandler> handler);
		UIEventHandler& getEventHandler();

	protected:
		virtual void draw(UIPainter& painter) const;
		virtual void update(Time t, bool moved);

		virtual void onFocus();
		virtual void onFocusLost();
		UIRoot* getRoot() override;

		void sendEvent(UIEvent&& event) const override;

		void playSound(const std::shared_ptr<const AudioClip>& clip);

	private:
		void setWidgetRect(Rect4f rect);
		void setParent(UIParent& parent);
		void doDestroy();

		UIParent* parent = nullptr;
		String id;

		Vector2f position;
		Vector2f size;
		Vector2f minSize;

		Vector4f innerBorder;
		Maybe<UISizer> sizer;

		std::shared_ptr<UIEventHandler> eventHandler;

		bool focused = false;
		bool mouseOver = false;
		bool positionUpdated = false;
	};
}
