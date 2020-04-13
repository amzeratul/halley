#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/maths/colour.h>
#include <halley/maths/vector4.h>
#include "halley/data_structures/maybe.h"
#include <halley/bytes/config_node_serializer.h>

namespace Halley
{
	class SpriteResource;
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
		Vector4f custom0;
		float rotation = 0;
		float textureRotation = 0;
		char _padding[8];
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
		static void drawMixedMaterials(const Sprite* sprites, size_t n, Painter& painter);

		Sprite& setMaterial(Resources& resources, String materialName = "");
		Sprite& setMaterial(std::shared_ptr<Material> m);
		Material& getMaterial() const { return *material; }
		bool hasMaterial() const { return material != nullptr; }

		Sprite& setImage(Resources& resources, String imageName, String materialName = "");
		Sprite& setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> material);
		Sprite& setImage(const SpriteResource& sprite, std::shared_ptr<const MaterialDefinition> material);
		Sprite& setImageData(const Texture& image);

		Sprite& setSprite(Resources& resources, String spriteSheetName, String imageName, String materialName = "");
		Sprite& setSprite(const SpriteResource& sprite, bool applyPivot = true);
		Sprite& setSprite(const SpriteSheet& sheet, String name, bool applyPivot = true);
		Sprite& setSprite(const SpriteSheetEntry& entry, bool applyPivot = true);

		Sprite& setPos(Vector2f pos) { vertexAttrib.pos = pos; return *this; }
		Sprite& setPosition(Vector2f pos) { vertexAttrib.pos = pos; return *this; }
		Vector2f getPosition() const { return vertexAttrib.pos; }
		Vector2f& getPosition() { return vertexAttrib.pos; }

		Sprite& setPivot(Vector2f pivot);
		Sprite& setAbsolutePivot(Vector2f pivot);
		Vector2f getPivot() const;
		Vector2f getAbsolutePivot() const;

		Sprite& setRotation(Angle1f angle);

		Sprite& setSize(Vector2f size);
		Sprite& setScale(Vector2f scale);
		Sprite& setScale(float scale);
		Sprite& scaleTo(Vector2f size);
		Vector2f getSize() const;
		Vector2f getScale() const;
		Vector2f getScaledSize() const;
		Vector2f getRawSize() const;
		Vector2f getOriginalSize() const;

		Sprite& setFlip(bool flip);
		bool isFlipped() const;

		Sprite& setColour(Colour4f colour) { vertexAttrib.colour = colour; return *this; }
		Colour4f getColour() const { return vertexAttrib.colour; }
		Colour4f& getColour() { return vertexAttrib.colour; }

		Sprite& setTexRect(Rect4f texRect);
		Rect4f getTexRect() const;

		Sprite& setCustom0(Vector4f custom0) { vertexAttrib.custom0 = custom0; return *this; }
		Vector4f getCustom0() const { return vertexAttrib.custom0; }
		Vector4f& getCustom0() { return vertexAttrib.custom0; }

		Sprite& setSliced(Vector4s slices);
		Sprite& setNotSliced();
		bool isSliced() const;
		Vector4s getSlices() const;

		Sprite& setVisible(bool visible) { this->visible = visible; return *this; }
		bool isVisible() const { return visible; }

		Sprite& setClip(Rect4f clip);
		Sprite& setAbsoluteClip(Rect4f clip);
		Sprite& setClip();
		std::optional<Rect4f> getClip() const;

		Rect4f getAABB() const;
		bool isInView(Rect4f rect) const;

		Vector4s getOuterBorder() const;
		Sprite& setOuterBorder(Vector4s border);

		Sprite clone() const;

	private:
		SpriteVertexAttrib vertexAttrib;
		std::shared_ptr<Material> material;

		Vector2f size;
		Vector4s slices;
		Vector4s outerBorder;
		std::optional<Rect4f> clip;
		bool absoluteClip = false;
		bool visible = true;
		bool flip = false;
		bool sliced = false;

		void computeSize();
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<Sprite> {
	public:
		Sprite deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}
