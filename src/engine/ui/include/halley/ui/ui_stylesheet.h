#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include <unordered_map>
#include <map>

namespace Halley {
	class UIStyleSheet;
	class UIColourScheme;
	class ConfigFile;
	class ConfigNode;
	class ConfigObserver;
	class AudioClip;
	class UISTyle;

	class UIStyleDefinition
	{
	public:
		UIStyleDefinition(String styleName, const ConfigNode& node, UIStyleSheet& styleSheet);
		~UIStyleDefinition();

		const String& getName() const;
		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		const String& getString(const String& name) const;
		float getFloat(const String& name) const;
		float getFloat(const String& name, float defaultValue) const;
		Vector2f getVector2f(const String& name) const;
		Vector2f getVector2f(const String& name, Vector2f defaultValue) const;
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
		UIStyleSheet& styleSheet;

		std::unique_ptr<Pimpl> pimpl;

		void loadDefaults();
	};

	class UIStyleSheet {
		friend class UIStyle;

	public:
		UIStyleSheet(Resources& resources);
		UIStyleSheet(Resources& resources, const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme = {});

		void load(const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme = {});

		bool updateIfNeeded();
		void reload(std::shared_ptr<const UIColourScheme> colourScheme);

		Resources& getResources() const { return resources; }
		const std::shared_ptr<const UIColourScheme>& getColourScheme() const { return lastColourScheme; }
		std::shared_ptr<const UIStyleDefinition> getStyle(const String& styleName) const;
		std::shared_ptr<UIStyleDefinition> getStyle(const String& styleName);

		const ConfigObserver& getStyleObserver(const String& styleName) const;
	private:
		Resources& resources;
		std::unordered_map<String, std::shared_ptr<UIStyleDefinition>> styles;
		std::map<String, ConfigObserver> observers;
		std::unordered_map<String, String> styleToObserver;
		std::shared_ptr<const UIColourScheme> lastColourScheme = nullptr;

		void load(const ConfigNode& node, const String& assetId, std::shared_ptr<const UIColourScheme> colourScheme);

		bool needsUpdate() const;
		void update();
	};
}
