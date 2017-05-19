#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/maths/colour.h>
#include <halley/maths/vector4.h>
#include "halley/data_structures/maybe.h"

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
		// See shared_assets/material/sprite_base.yaml for reference
		Vector4f vertPos;
		Vector2f pos;
		Vector2f pivot;
		Vector2f size;
		Vector2f scale;
		Colour4f colour;
		Rect4f texRect;
		float rotation = 0;
		float textureRotation = 0;
	};

	class Sprite
	{
	public:
		Sprite();

		void draw(Painter& painter) const;
		void drawNormal(Painter& painter) const;
		void drawSliced(Painter& painter) const;
		void drawSliced(Painter& painter, Vector4s slices) const;
		static void draw(const Sprite* sprites, size_t n, Painter& painter);

		Rect4f getAABB() const;
		bool isInView(Rect4f rect) const;
		Vector2f getScaledSize() const;
		
		Sprite& setMaterial(Resources& resources, String materialName = "");
		Sprite& setMaterial(std::shared_ptr<Material> m);
		Material& getMaterial() const { return *material; }
		bool hasMaterial() const { return material != nullptr; }

		Sprite& setImage(Resources& resources, String imageName, String materialName = "");
		Sprite& setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> material);
		Sprite& setImageData(const Texture& image);

		Vector2f getPosition() const;
		Colour4f getColour() const;

		Sprite& setPos(Vector2f pos);
		Sprite& setRotation(Angle1f angle);
		Sprite& setColour(Colour4f colour);
		Sprite& setScale(Vector2f scale);
		Sprite& setScale(float scale);
		Sprite& scaleTo(Vector2f size);
		Sprite& setFlip(bool flip);

		Sprite& setPivot(Vector2f pivot);
		Sprite& setAbsolutePivot(Vector2f pivot);
		Sprite& setSize(Vector2f size);
		Sprite& setTexRect(Rect4f texRect);

		Sprite& setSprite(Resources& resources, String spriteSheetName, String imageName, String materialName = "");
		Sprite& setSprite(const SpriteSheet& sheet, String name);
		Sprite& setSprite(const SpriteSheetEntry& entry, bool applyPivot = true);

		Sprite& setSliced(Vector4s slices);
		Sprite& setSlicedFromMaterial();
		Sprite& setNormal();

		Sprite& setClip(Rect4f clip);
		Sprite& setClip();
		Maybe<Rect4f> getClip() const;

		Sprite clone() const;

	private:
		std::shared_ptr<Material> material;
		SpriteVertexAttrib vertexAttrib;

		Vector2f size;
		Vector4s slices;
		bool flip = false;
		bool sliced = false;
		Maybe<Rect4f> clip;

		void computeSize();
	};
}
