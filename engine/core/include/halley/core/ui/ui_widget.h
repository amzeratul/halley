#pragma once
#include "ui_sizer.h"
#include "ui_root.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class Painter;

	class UIWidget : public IUIElement, public UIParent {
		friend class UIParent;

	public:
		UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget();

		void doDraw(UIPainter& painter) const;
		void doUpdate(Time t);

		Vector2f computeMinimumSize() const override;
		void setRect(Rect4f rect) override;

		void layout();
		
		Maybe<UISizer>& getSizer();
		const Maybe<UISizer>& getSizer() const;

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

		void setFocused(bool focused);
		void setMouseOver(bool mouseOver);
		virtual void pressMouse(int button);
		virtual void releaseMouse(int button);

		UIRoot& getRoot() override;
		void destroy();

	protected:
		virtual void draw(UIPainter& painter) const;
		virtual void update(Time t, bool moved);

	private:
		void setWidgetRect(Rect4f rect);
		void setParent(UIParent& parent);

		UIParent* parent = nullptr;
		UIRoot* uiRoot = nullptr;
		String id;

		Vector2f position;
		Vector2f size;
		Vector2f minSize;

		Vector4f innerBorder;
		Maybe<UISizer> sizer;

		bool focused = false;
		bool mouseOver = false;
		bool positionUpdated = false;
	};
}
