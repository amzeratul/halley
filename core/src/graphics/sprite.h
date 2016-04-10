#pragma once

namespace Halley
{
	class Material;

	struct SpriteVertexAttrib
	{
		Vector2f pos;
		Vector2f offset;
		Vector2f size;
		Vector2f rotation;
		Colour4f colour;
		Rect4f texRect;
		Vector2f vertPos;
	};

	class Sprite
	{
	public:
		Sprite();

		void draw(Painter& painter) const;
		bool isInView(Rect4f rect) const;
		
		void setMaterial(std::shared_ptr<Material> m) { material = m; }
		std::shared_ptr<Material> getMaterial() const { return material; }

		void setPos(Vector2f pos);
		void setRotation(Angle1f angle);
		void setColour(Colour4f colour);
		void setScale(Vector2f scale);

		void setOffset(Vector2f offset);
		void setSize(Vector2f size);
		void setTexRect(Rect4f texRect);

	private:
		std::shared_ptr<Material> material;
		mutable std::array<SpriteVertexAttrib, 4> vertices;

		Vector2f scale;
		Vector2f size;
		mutable bool dirty = false;

		void update() const;
	};
}