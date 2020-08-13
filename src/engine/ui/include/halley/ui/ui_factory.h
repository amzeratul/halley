#pragma once

#include "ui_sizer.h"
#include "ui_stylesheet.h"
#include <memory>
#include <map>
#include <functional>
#include "ui_input.h"
#include "halley/text/i18n.h"

namespace Halley
{
	class HalleyAPI;
	class ConfigNode;
	class Resources;
	class I18N;
	class UIWidget;
	class UIStyle;
	class IClipboard;
	class InputKeyboard;

	class UIFactory
	{
	public:
		using WidgetFactory = std::function<std::shared_ptr<UIWidget>(const ConfigNode&)>;

		UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet);
		UIFactory(const UIFactory& other) = delete;
		UIFactory(UIFactory&& other) = delete;
		virtual ~UIFactory();

		UIFactory& operator=(const UIFactory& other) = delete;
		UIFactory& operator=(UIFactory&& other) = delete;

		void addFactory(const String& key, WidgetFactory factory);
		
		void pushConditions(std::vector<String> conditions);
		void popConditions();

		std::shared_ptr<UIWidget> makeUI(const String& configName);
		std::shared_ptr<UIWidget> makeUI(const String& configName, std::vector<String> conditions);
		std::shared_ptr<UIWidget> makeUIFromNode(const ConfigNode& node);

		void loadUI(UIWidget& target, const String& configName);
		void loadUI(UIWidget& target, const ConfigFile& configFile);

		void setInputButtons(const String& key, UIInputButtons buttons);
		void applyInputButtons(UIWidget& widget, const String& key);

		UIStyle getStyle(const String& name) const;
		std::shared_ptr<UIStyleSheet> getStyleSheet() const;

		Resources& getResources() const;

		std::unique_ptr<UIFactory> withResources(Resources& newResources) const;

		const I18N& getI18N() const;

	protected:
		struct ParsedOption {
			String id;
			LocalisedString text;
			String image;
			String inactiveImage;
			String spriteSheet;
			String sprite;
			LocalisedString tooltip;
			Vector4f border;
			bool active;
		};
		
		const HalleyAPI& api;
		Resources& resources;
		const I18N& i18n;

		std::shared_ptr<UIWidget> makeWidget(const ConfigNode& node);
		std::shared_ptr<UISizer> makeSizerPtr(const ConfigNode& node);
		std::optional<UISizer> makeSizer(const ConfigNode& node);
		UISizer makeSizerOrDefault(const ConfigNode& node, UISizer&& defaultSizer);
		void loadSizerChildren(UISizer& sizer, const ConfigNode& node);

		static std::optional<Vector2f> asMaybeVector2f(const ConfigNode& node);
		static Vector2f asVector2f(const ConfigNode& node, std::optional<Vector2f> defaultValue);
		static Vector4f asVector4f(const ConfigNode& node, std::optional<Vector4f> defaultValue);
		LocalisedString parseLabel(const ConfigNode& node, const String& defaultOption = "", const String& key = "text");
		std::vector<ParsedOption> parseOptions(const ConfigNode& node);

		std::shared_ptr<UIWidget> makeBaseWidget(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeLabel(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeButton(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTextInput(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeSpinControl(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeList(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeDropdown(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeCheckbox(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeImage(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeAnimation(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeScrollPane(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeScrollBar(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeScrollBarPane(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeSlider(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeHorizontalDiv(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeVerticalDiv(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTabbedPane(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makePagedPane(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeFramedImage(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeHybridList(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeSpinList(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeOptionListMorpher(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeTreeList(const ConfigNode& node);

		bool hasCondition(const String& condition) const;
		bool resolveConditions(const ConfigNode& node) const;

	private:
		std::shared_ptr<UIStyleSheet> styleSheet;
		std::vector<String> conditions;
		std::vector<size_t> conditionStack;

		std::map<String, WidgetFactory> factories;
		std::map<String, UIInputButtons> inputButtons;
	};
}
