#pragma once

#include <halley/maths/vector2d.h>
#include <halley/maths/rect.h>
#include <halley/maths/colour.h>

namespace Halley
{
	class Resources;
	class SpriteSheetEntry;
	class SpriteSheet;
	class Material;
	class Texture;
	class MaterialDefinition;
	class Painter;

	struct SpriteVertexAttrib
	{
		// This structure must match the layout of the shader
		// See sprite.yaml for reference
		Vector2f pos;
		Vector2f pivot;     // pos and offset are a single vec4 on the shader
		Vector2f size;
		Vector2f rotation;   // size and rotation are a single vec4 on the shader
		Colour4f colour;
		Rect4f texRect;
		Vector2f vertPos;
	};

	class Sprite
	{
	public:
		Sprite();

		void draw(Painter& painter) const;
		static void draw(const Sprite* sprites, size_t n, Painter& painter);

		bool isInView(Rect4f rect) const;
		
		Sprite& setMaterial(std::shared_ptr<Material> m) { material = m; return *this; }
		Material& getMaterial() const { return *material; }

		Sprite& setImage(Resources& resources, String imageName, String materialName = "");
		Sprite& setImage(std::shared_ptr<Texture> image, std::shared_ptr<MaterialDefinition> material);

		Vector2f getPosition() const;

		Sprite& setPos(Vector2f pos);
		Sprite& setRotation(Angle1f angle);
		Sprite& setColour(Colour4f colour);
		Sprite& setScale(Vector2f scale);
		Sprite& setScale(float scale);
		Sprite& setFlip(bool flip);

		Sprite& setPivot(Vector2f offset);
		Sprite& setSize(Vector2f size);
		Sprite& setTexRect(Rect4f texRect);

		Sprite& setSprite(const SpriteSheet& sheet, String name);
		Sprite& setSprite(const SpriteSheetEntry& entry);

	private:
		std::shared_ptr<Material> material;
		mutable std::array<SpriteVertexAttrib, 4> vertices;

		Vector2f scale;
		Vector2f size;
		bool flip = false;
		mutable bool dirty = false;

		void update() const;
		void computeSize();
	};
}