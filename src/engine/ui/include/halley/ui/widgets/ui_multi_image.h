#pragma once

#include "../ui_widget.h"

namespace Halley
{
	class UIMultiImage : public UIWidget
	{
	public:
		UIMultiImage(const String& id, Vector2f size, Vector<Sprite> sprites, Vector<Vector2f> offsets = {});

		Sprite& getSprite(size_t index);
		const Sprite& getSprite(size_t index) const;
		const Vector<Sprite>& getSprites() const;
		void setSprite(size_t index, Sprite sprite);
		void setSprites(Vector<Sprite> sprites);
		
		Vector2f getOffset(size_t index) const;
		const Vector<Vector2f>& getOffsets() const;
		void setOffset(size_t index, Vector2f offset);
		void setOffsets(Vector<Vector2f> offsets);
		
	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		Vector<Sprite> sprites;
		Vector<Vector2f> offsets;

		bool dirty = true;
	};
}
