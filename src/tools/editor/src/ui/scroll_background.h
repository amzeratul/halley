#pragma once

namespace Halley {
    class ScrollBackground : public UIWidget {
    public:
        ScrollBackground(String id, Resources& res, UISizer sizer);

    protected:
        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

    private:
		Sprite bg;
    };
}
