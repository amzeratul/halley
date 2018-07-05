#pragma once

#include "ui_sizer.h"
#include "ui_stylesheet.h"
#include <memory>
#include <map>
#include <functional>
#include "ui_input.h"

namespace Halley
{
	class HalleyAPI;
	class ConfigNode;
	class Resources;
	class I18N;
	class UIWidget;
	class UIStyle;
	class Clipboard;

	class UIFactory
	{
	public:
		using WidgetFactory = std::function<std::shared_ptr<UIWidget>(const ConfigNode&)>;

		UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet);

		void addFactory(const String& key, WidgetFactory factory);
		
		void pushConditions(std::vector<String> conditions);
		void popConditions();

		std::shared_ptr<UIWidget> makeUI(const String& configName);
		std::shared_ptr<UIWidget> makeUI(const String& configName, std::vector<String> conditions);
		std::shared_ptr<UIWidget> makeUIFromNode(const ConfigNode& node);

		void setInputButtons(const String& key, UIInputButtons buttons);
		void applyInputButtons(UIWidget& widget, const String& key);

		UIStyle getStyle(const String& name) const;
		std::shared_ptr<UIStyleSheet> getStyleSheet() const;
		std::shared_ptr<Clipboard> getClipboard() const;

	protected:
		const HalleyAPI& api;
		Resources& resources;
		const I18N& i18n;
		
		std::shared_ptr<UIWidget> makeWidget(const ConfigNode& node);
		std::shared_ptr<UISizer> makeSizerPtr(const ConfigNode& node);
		Maybe<UISizer> makeSizer(const ConfigNode& node);
		UISizer makeSizerOrDefault(const ConfigNode& node, UISizer&& defaultSizer);
		void loadSizerChildren(UISizer& sizer, const ConfigNode& node);

		static Maybe<Vector2f> asMaybeVector2f(const ConfigNode& node);
		static Vector2f asVector2f(const ConfigNode& node, Maybe<Vector2f> defaultValue);
		static Vector4f asVector4f(const ConfigNode& node, Maybe<Vector4f> defaultValue);
		LocalisedString parseLabel(const ConfigNode& node, const String& defaultOption = "", const String& key = "text");
		std::vector<std::pair<String, LocalisedString>> parseOptions(const ConfigNode& node);

		std::shared_ptr<UIWidget> makeBaseWidget(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeLabel(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeButton(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTextInput(const ConfigNode& node);
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
		std::shared_ptr<UIWidget> makeFramedImage(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeHybridList(const ConfigNode& entryNode);

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
