#pragma once
#include "../ui_widget.h"

namespace Halley {
	class UICustomPaint : public UIWidget {
	public:
        using DrawCallback = std::function<void(UIPainter&, Rect4f)>;

		explicit UICustomPaint(String id, std::optional<UISizer> sizer = {}, DrawCallback callback = {});

        void setCallback(DrawCallback callback);

		void draw(UIPainter& painter) const override;

    private:
        DrawCallback callback;
    };
}
