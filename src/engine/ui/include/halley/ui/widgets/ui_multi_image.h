#pragma once

#include "../ui_widget.h"

namespace Halley
{
	class UIMultiImage : public UIWidget
	{
	public:
		UIMultiImage(const String& id, Vector2f size, std::vector<Sprite> sprites, std::vector<Vector2f> offsets = {});

		Sprite& getSprite(size_t index);
		const Sprite& getSprite(size_t index) const;
		const std::vector<Sprite>& getSprites() const;
		void setSprite(size_t index, Sprite sprite);
		void setSprites(std::vector<Sprite> sprites);
		
		Vector2f getOffset(size_t index) const;
		const std::vector<Vector2f>& getOffsets() const;
		void setOffset(size_t index, Vector2f offset);
		void setOffsets(std::vector<Vector2f> offsets);
		
	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		std::vector<Sprite> sprites;
		std::vector<Vector2f> offsets;

		bool dirty = true;
	};
}
