#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/data_structures/flat_map.h"
#include <map>

namespace Halley {
	class ConfigFile;
	class ConfigNode;
	class ConfigObserver;
	class AudioClip;
	class UISTyle;

	class UIStyleDefinition
	{
	public:
		UIStyleDefinition(String styleName, const ConfigNode& node, Resources& resources);

		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		std::shared_ptr<const AudioClip> getAudioClip(const String& name) const;
		float getFloat(const String& name) const;
		std::shared_ptr<const UIStyleDefinition> getSubStyle(const String& name) const;

	private:
		const String styleName;
		const ConfigNode& node;
		Resources& resources;

		mutable FlatMap<String, Sprite> sprites;
		mutable FlatMap<String, TextRenderer> textRenderers;
		mutable FlatMap<String, Vector4f> borders;
		mutable FlatMap<String, std::shared_ptr<const AudioClip>> audioClips;
		mutable FlatMap<String, float> floats;
		mutable FlatMap<String, std::shared_ptr<const UIStyleDefinition>> subStyles;
	};

	class UIStyleSheet {
		friend class UIStyle;

	public:
		UIStyleSheet(Resources& resources);
		UIStyleSheet(Resources& resources, const ConfigFile& file);

		void load(const ConfigFile& file);

		bool needsUpdate() const;
		void update();

	private:
		Resources& resources;
		FlatMap<String, std::shared_ptr<UIStyleDefinition>> styles;
		std::map<String, ConfigObserver> observers;

		void load(const ConfigNode& node);
		std::shared_ptr<const UIStyleDefinition> getStyle(const String& styleName) const;
	};
}
