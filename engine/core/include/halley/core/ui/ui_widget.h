#pragma once
#include "ui_sizer.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class UIWidget : public IUISizeable {
	public:
		UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget() {}

		Vector2f computeMinimumSize() override;
		void setRect(Rect4f rect) override;
		
		Maybe<UISizer>& getSizer();
		const Maybe<UISizer>& getSizer() const;

		bool isFocusable() const;
		String getId() const;

		Vector2f getMinimumSize() const;
		Vector4f getInnerBorder() const;

	protected:
		void setWidgetRect(Rect4f);

		String id;

		Vector2f position;
		Vector2f size;
		Vector2f minSize;

		Vector4f innerBorder;
		Maybe<UISizer> sizer;

		bool focusable = false;
	};
}
