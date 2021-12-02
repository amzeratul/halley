#pragma once

#include "ui_sizer.h"
#include "ui_stylesheet.h"
#include <memory>
#include <map>
#include <functional>

#include "ui_colour_scheme.h"
#include "ui_input.h"
#include "halley/text/i18n.h"

namespace Halley
{
	class UIList;
	class UIDefinition;
	class HalleyAPI;
	class ConfigNode;
	class Resources;
	class I18N;
	class UIWidget;
	class UILabel;
	class UIStyle;
	class IClipboard;
	class InputKeyboard;

	class UIFactoryWidgetProperties {
	public:
		struct Entry {
            String label;
	        String name;
            String type;
            std::vector<String> defaultValue;

			Entry() = default;
			Entry(String label, String name, String type, std::vector<String> defaultValue = {});
			Entry(String label, String name, String type, String defaultValue);
        };

		std::vector<Entry> entries;
		std::vector<Entry> childEntries;
		String name;
		String iconName;
		String childName;
		bool canHaveChildren = true;
	};

	class UIFactory
	{
	public:
		using WidgetFactory = std::function<std::shared_ptr<UIWidget>(const ConfigNode&)>;
		using ConstructionCallback = std::function<void(const std::shared_ptr<IUIElement>&, const String& uuid)>;

		UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet = {}, std::shared_ptr<const UIColourScheme> colourScheme = {});
		UIFactory(const UIFactory& other) = delete;
		UIFactory(UIFactory&& other) = delete;
		virtual ~UIFactory();

		UIFactory& operator=(const UIFactory& other) = delete;
		UIFactory& operator=(UIFactory&& other) = delete;

		void addFactory(const String& key, WidgetFactory factory, UIFactoryWidgetProperties properties = {});
		
		void pushConditions(std::vector<String> conditions);
		void popConditions();

		std::shared_ptr<UIWidget> makeUI(const String& configName);
		std::shared_ptr<UIWidget> makeUI(const String& configName, std::vector<String> conditions);
		std::shared_ptr<UIWidget> makeUI(const UIDefinition& definition);

		void loadUI(UIWidget& target, const String& configName);
		void loadUI(UIWidget& target, const UIDefinition& uiDefinition);

		const UIFactoryWidgetProperties& getPropertiesForWidget(const String& widgetClass) const;
		UIFactoryWidgetProperties getGlobalWidgetProperties() const;
		std::vector<String> getWidgetClassList() const;

		void setInputButtons(const String& key, UIInputButtons buttons);
		void applyInputButtons(UIWidget& widget, const String& key);

		UIStyle getStyle(const String& name) const;
		std::shared_ptr<UIStyleSheet> getStyleSheet() const;

		Resources& getResources() const;

		std::unique_ptr<UIFactory> clone() const;
		std::unique_ptr<UIFactory> cloneWithResources(Resources& newResources) const;
		virtual std::unique_ptr<UIFactory> make(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet = {}, std::shared_ptr<const UIColourScheme> colourScheme = {}) const;

		const I18N& getI18N() const;

		void setStyleSheet(std::shared_ptr<UIStyleSheet> styleSheet);
		std::shared_ptr<const UIColourScheme> getColourScheme() const;

		void update();

		virtual Sprite makeAssetTypeIcon(AssetType type) const;

		void setConstructionCallback(ConstructionCallback callback);

		static UISizerAlignFlags::Type parseSizerAlignFlags(const ConfigNode& node, UISizerAlignFlags::Type defaultValue = UISizerAlignFlags::Fill);
		static ConfigNode makeSizerAlignFlagsNode(UISizerAlignFlags::Type align);
		static UISizerAlignFlags::Type normalizeDirection(UISizerAlignFlags::Type align, bool removeFill);

	protected:
		struct ParsedOption {
			String id;
			LocalisedString text;
			String image;
			String iconColour;
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
		std::shared_ptr<const UIColourScheme> colourScheme;

		std::shared_ptr<IUIElement> makeWidget(const ConfigNode& node);
		std::optional<UISizer> makeSizer(const ConfigNode& node);
		UISizer makeSizerOrDefault(const ConfigNode& node, UISizer&& defaultSizer);
		void loadSizerChildren(UISizer& sizer, const ConfigNode& node);

		LocalisedString parseLabel(const ConfigNode& node, const String& defaultOption = "", const String& key = "text");
		std::vector<ParsedOption> parseOptions(const ConfigNode& node);

		std::shared_ptr<UIWidget> makeBaseWidget(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeLabel(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeButton(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTextInput(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeSpinControl(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeSpinControl2(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeDropdown(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeCheckbox(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeImage(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeMultiImage(const ConfigNode& node);
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
		std::shared_ptr<UIWidget> makeDebugConsole(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeList(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTreeList(const ConfigNode& node);
		void applyListProperties(UIList& list, const ConfigNode& widgetNode, const String& inputConfigName);

		UIFactoryWidgetProperties getBaseWidgetProperties() const;
		UIFactoryWidgetProperties getLabelProperties() const;
		UIFactoryWidgetProperties getButtonProperties() const;
		UIFactoryWidgetProperties getTextInputProperties() const;
		UIFactoryWidgetProperties getSpinControlProperties() const;
		UIFactoryWidgetProperties getSpinControl2Properties() const;
		UIFactoryWidgetProperties getDropdownProperties() const;
		UIFactoryWidgetProperties getCheckboxProperties() const;
		UIFactoryWidgetProperties getImageProperties() const;
		UIFactoryWidgetProperties getMultiImageProperties() const;
		UIFactoryWidgetProperties getAnimationProperties() const;
		UIFactoryWidgetProperties getScrollPaneProperties() const;
		UIFactoryWidgetProperties getScrollBarProperties() const;
		UIFactoryWidgetProperties getScrollBarPaneProperties() const;
		UIFactoryWidgetProperties getSliderProperties() const;
		UIFactoryWidgetProperties getHorizontalDivProperties() const;
		UIFactoryWidgetProperties getVerticalDivProperties() const;
		UIFactoryWidgetProperties getTabbedPaneProperties() const;
		UIFactoryWidgetProperties getPagedPaneProperties() const;
		UIFactoryWidgetProperties getFramedImageProperties() const;
		UIFactoryWidgetProperties getHybridListProperties() const;
		UIFactoryWidgetProperties getSpinListProperties() const;
		UIFactoryWidgetProperties getOptionListMorpherProperties() const;
		UIFactoryWidgetProperties getDebugConsoleProperties() const;
		UIFactoryWidgetProperties getListProperties() const;
		UIFactoryWidgetProperties getTreeListProperties() const;
		
		bool hasCondition(const String& condition) const;
		bool resolveConditions(const ConfigNode& node) const;

		Colour4f getColour(const String& key) const;
		void loadDefaultColourScheme();

	private:
		std::shared_ptr<UIStyleSheet> styleSheet;
		std::vector<String> conditions;
		std::vector<size_t> conditionStack;

		std::map<String, WidgetFactory> factories;
		std::map<String, UIFactoryWidgetProperties> properties;
		std::map<String, UIInputButtons> inputButtons;

		ConstructionCallback constructionCallback;
	};
}
