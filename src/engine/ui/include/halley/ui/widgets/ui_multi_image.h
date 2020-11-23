#pragma once

#include "../ui_widget.h"

namespace Halley
{
	class AnimationPlayer;

	class UIMultiImage : public UIWidget
	{
	public:
		UIMultiImage(const String& id, Vector2f size, std::vector<Sprite> sprites, std::vector<Vector2f> offsets = {});

		Sprite& getSprite(const int index);
		const Sprite& getSprite(const int index) const;
		const std::vector<Sprite>& getSprites() const;
		void setSprite(const int index, Sprite sprite);
		void setSprites(std::vector<Sprite> sprites);
		
		Vector2f getOffset(const int index) const;
		const std::vector<Vector2f>& getOffsets() const;
		void setOffset(const int index, Vector2f offset);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		std::vector<Sprite> sprites;
		std::vector<Vector2f> offsets;

		bool dirty = true;
	};
}
