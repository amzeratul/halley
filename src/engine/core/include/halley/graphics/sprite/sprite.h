#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/maths/colour.h>
#include <halley/maths/vector4.h>
#include <halley/bytes/config_node_serializer_base.h>

namespace Halley
{
	class VideoAPI;
	class Image;
	class SpriteHotReloader;
	class SpriteResource;
	class Resources;
	class SpriteSheetEntry;
	class SpriteSheet;
	class Material;
	class Texture;
	class MaterialDefinition;
	class MaterialUpdater;
	class Painter;

	struct SpriteVertexAttrib
	{
		// This structure must match the layout of the shader
		// See shared_assets/material/sprite_base.material for reference
		//Vector4f vertPos; // A hack in getVertexAttrib() allows this field to be omitted as long as something else is here
		Vector2f pos;
		Vector2f pivot;
		Vector2f size;
		Vector2f scale;
		Colour4f colour;
		Rect4f texRect0;
		Rect4f texRect1;
		Vector4f custom0;
		Vector4f custom1;
		Vector4f custom2;
		float rotation = 0;
		float textureRotation = 0;
		char _padding[8];
	};

	class Sprite
	{
	public:
		struct RectInfo {
			Vector2f pivot;
			Vector2f size;
			Rect4f texRect0;
			Rect4f texRect1;
		};
		
		Sprite();

		void draw(Painter& painter, const std::optional<Rect4f>& extClip = {}) const;
		void drawNormal(Painter& painter, const std::optional<Rect4f>& extClip = {}) const;
		void drawSliced(Painter& painter, const std::optional<Rect4f>& extClip = {}) const;
		void drawSliced(Painter& painter, Vector4s slices, const std::optional<Rect4f>& extClip = {}) const;
		static void draw(gsl::span<const Sprite> sprites, Painter& painter);
		static void drawMixedMaterials(const Sprite* sprites, size_t n, Painter& painter);

		Sprite& setMaterial(Resources& resources, String materialName = "");
		Sprite& setMaterial(std::shared_ptr<const Material> m);
		MaterialUpdater getMutableMaterial();
		const Material& getMaterial() const
		{
			Expects(material);
			return *material;
		}
		const std::shared_ptr<const Material>& getMaterialPtr() const { return material; }
		bool hasMaterial() const { return material != nullptr; }
		bool hasCompatibleMaterial(const Material& other) const;

		Sprite& setImage(Resources& resources, std::string_view imageName, std::string_view materialName = "");
		Sprite& setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> material);
		Sprite& setImage(const SpriteResource& sprite, std::shared_ptr<const MaterialDefinition> material);
		Sprite& setImage(Resources& resources, VideoAPI& videoAPI, std::shared_ptr<Image> image, std::string_view materialName = "");
		Sprite& setImageData(const Texture& image);

		Sprite& setSprite(Resources& resources, std::string_view spriteSheetName, std::string_view imageName, std::string_view materialName = "");
		Sprite& setSprite(const SpriteResource& sprite, bool applyPivot = true);
		Sprite& setSprite(const SpriteSheet& sheet, std::string_view name, bool applyPivot = true);
		Sprite& setSprite(const SpriteSheetEntry& entry, bool applyPivot = true);

		Sprite& setPos(Vector2f pos) { Expects(pos.isValid()); vertexAttrib.pos = pos; return *this; }
		Sprite& setPosition(Vector2f pos) & { Expects(pos.isValid()); vertexAttrib.pos = pos; return *this; }
		Sprite&& setPosition(Vector2f pos) && { Expects(pos.isValid()); vertexAttrib.pos = pos; return std::move(*this); }
		Vector2f getPosition() const { return vertexAttrib.pos; }
		Vector2f& getPosition() { return vertexAttrib.pos; }

		Sprite& setPivot(Vector2f pivot);
		Sprite& setAbsolutePivot(Vector2f pivot);
		Vector2f getPivot() const { return vertexAttrib.pivot; }
		Vector2f getAbsolutePivot() const;
		Vector2f getUncroppedAbsolutePivot() const;

		Sprite& setRotation(Angle1f angle);
		Angle1f getRotation() const { return Angle1f::fromRadians(vertexAttrib.rotation, false); }

		Sprite& setSize(Vector2f size);
		Sprite& setScale(Vector2f scale);
		Sprite& setScale(float scale);
		Sprite& setSliceScale(float scale);
		Sprite& scaleTo(Vector2f size);
		Vector2f getSize() const { return size; }
		Vector2f getScale() const { return vertexAttrib.scale; }
		Vector2f getScaledSize() const { return size * vertexAttrib.scale; }
		Vector2f getUncroppedSize() const;
		Vector2f getUncroppedScaledSize() const;

		Sprite& setFlip(bool flip);
		bool isFlipped() const { return flip; }

		Sprite& setColour(Colour4f colour) { vertexAttrib.colour = colour; return *this; }
		Colour4f getColour() const { return vertexAttrib.colour; }
		Colour4f& getColour() { return vertexAttrib.colour; }
		Sprite& setAlpha(float alpha) { vertexAttrib.colour.a = alpha; return *this; }
		float getAlpha() const { return vertexAttrib.colour.a; }

		Sprite& setTexRect(Rect4f texRect);
		Rect4f getTexRect() const { return vertexAttrib.texRect0; }
		Sprite& setTexRect0(Rect4f texRect);
		Rect4f getTexRect0() const { return vertexAttrib.texRect0; }
		Sprite& setTexRect1(Rect4f texRect) { vertexAttrib.texRect1 = texRect; return *this; }
		Rect4f getTexRect1() const { return vertexAttrib.texRect1; }

		Sprite& setCustom0(Vector4f custom0) { vertexAttrib.custom0 = custom0; return *this; }
		Vector4f getCustom0() const { return vertexAttrib.custom0; }
		Vector4f& getCustom0() { return vertexAttrib.custom0; }
		Sprite& setCustom1(Vector4f custom1) { vertexAttrib.custom1 = custom1; return *this; }
		Vector4f getCustom1() const { return vertexAttrib.custom1; }
		Vector4f& getCustom1() { return vertexAttrib.custom1; }
		Sprite& setCustom2(Vector4f custom2) { vertexAttrib.custom2 = custom2; return *this; }
		Vector4f getCustom2() const { return vertexAttrib.custom2; }
		Vector4f& getCustom2() { return vertexAttrib.custom2; }
		
		Sprite& setSliced(Vector4s slices);
		Sprite& setNotSliced();
		bool isSliced() const { return sliced; }
		Vector4s getSlices() const;

		Sprite& setVisible(bool visible) { this->visible = visible; return *this; }
		bool isVisible() const { return visible; }

		bool hasPointVisible(Rect4f area) const;

		Sprite& setClip(Rect4f clip);
		Sprite& setAbsoluteClip(Rect4f clip);
		Sprite& setClip();
		std::optional<Rect4f> getClip() const;
		std::optional<Rect4f> getAbsoluteClip() const;

		Sprite& crop(Vector4f sides);
		RectInfo getRectInfo() const;
		void setRectInfo(const RectInfo& info);

		Rect4f getLocalAABB() const;
		Rect4f getAABB() const;
		Rect4f getUncroppedAABB() const;
		
		bool isInView(Rect4f rect) const;

		Vector4s getOuterBorder() const { return outerBorder; }
		Sprite& setOuterBorder(Vector4s border);

		Sprite clone() const;

		bool operator==(const Sprite& other) const;
		bool operator!=(const Sprite& other) const;

	private:
		Vector2f size;
		bool visible : 1;
		bool flip : 1;
		bool hasClip : 1;
		bool absoluteClip : 1;
		bool sliced : 1;
		bool rotated : 1;
		float sliceScale = 1;

		std::shared_ptr<const Material> material;
		SpriteVertexAttrib vertexAttrib;

		Vector4s slices;
		Vector4s outerBorder;
		Rect4f clip; // This is not a std::optional<Rect4f>, and instead has a companion bool, because it allows for better memory alignment

		void doSetSprite(const SpriteSheetEntry& entry, bool applyPivot);
		void computeSize();

		const void* getVertexAttrib() const;

		template<typename F> void paintWithClip(Painter& painter, const std::optional<Rect4f>& clip, F f) const;

#ifdef ENABLE_HOT_RELOAD
	public:
		Sprite(const Sprite& other);
		Sprite(Sprite&& other) noexcept;
		Sprite& operator=(const Sprite& other);
		Sprite& operator=(Sprite&& other) noexcept;
		~Sprite();

		void reloadSprite(const SpriteResource& sprite);
		bool hasLastAppliedPivot() const;
		void clearSpriteSheetRef();

	private:
		bool lastAppliedPivot = false;
		uint32_t hotReloadIdx = 0;
		const SpriteHotReloader* hotReloadRef = nullptr;

		void setHotReload(const SpriteHotReloader* ref, uint32_t index);
		std::shared_ptr<Material> copyMaterialParameters(const std::shared_ptr<const Material>& oldMaterial, std::shared_ptr<Material> newMaterial);
#else
	public:
		Sprite(const Sprite& other) = default;
		Sprite(Sprite&& other) noexcept = default;
		Sprite& operator=(const Sprite& other) = default;
		Sprite& operator=(Sprite&& other) noexcept = default;
#endif

	};

	class Resources;
	template<>
	class ConfigNodeSerializer<Sprite> {
	public:
		ConfigNode serialize(const Sprite& sprite, const EntitySerializationContext& context);
		Sprite deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, Sprite& target);
	};
}
