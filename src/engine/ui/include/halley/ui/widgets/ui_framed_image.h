#pragma once

#include "ui_image.h"

namespace Halley {
    class UIFramedImage : public UIImage {
    public:
		UIFramedImage(const String& id, UIStyle style, Maybe<UISizer> sizer = {});
		UIFramedImage(const String& id, Sprite frame, Sprite framedImage, Vector4f frameBorder, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});

	    void draw(UIPainter& painter) const override;
	    void update(Time t, bool moved) override;

		void setFramedSprite(const Sprite& sprite);
		Sprite& getFramedSprite();
		const Sprite& getFramedSprite() const;

		void setScrolling(Vector2f scrollSpeed, Maybe<Vector2f> startPos = {});

    private:
		Sprite framedSprite;

		Vector2f scrollPos;
		Vector2f scrollSpeed;
		Vector4f frameBorder;
    };
}
