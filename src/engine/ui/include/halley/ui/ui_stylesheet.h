#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include <unordered_map>
#include <map>

namespace Halley {
	class UIColourScheme;
	class ConfigFile;
	class ConfigNode;
	class ConfigObserver;
	class AudioClip;
	class UISTyle;

	class UIStyleDefinition
	{
	public:
		UIStyleDefinition(String styleName, const ConfigNode& node, Resources& resources, UIColourScheme* colourScheme);
		~UIStyleDefinition();

		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		const String& getString(const String& name) const;
		float getFloat(const String& name) const;
		Colour4f getColour(const String& name) const;
		std::shared_ptr<const UIStyleDefinition> getSubStyle(const String& name) const;

		bool hasTextRenderer(const String& name) const;
		bool hasColour(const String& name) const;
		bool hasSubStyle(const String& name) const;

		void reload(const ConfigNode& node);

	private:
		class Pimpl;
		
		const String styleName;
		const ConfigNode* node = nullptr;
		Resources& resources;
		UIColourScheme* colourScheme = nullptr;

		std::unique_ptr<Pimpl> pimpl;
	};

	class UIStyleSheet {
		friend class UIStyle;

	public:
		UIStyleSheet(Resources& resources);
		UIStyleSheet(Resources& resources, const ConfigFile& file, UIColourScheme* colourScheme = nullptr);

		void load(const ConfigFile& file, UIColourScheme* colourScheme = nullptr);

		bool updateIfNeeded();

	private:
		Resources& resources;
		std::unordered_map<String, std::shared_ptr<UIStyleDefinition>> styles;
		std::map<String, ConfigObserver> observers;
		UIColourScheme* lastColourScheme = nullptr;

		void load(const ConfigNode& node, UIColourScheme* colourScheme);
		std::shared_ptr<const UIStyleDefinition> getStyle(const String& styleName) const;

		bool needsUpdate() const;
		void update();
	};
}
