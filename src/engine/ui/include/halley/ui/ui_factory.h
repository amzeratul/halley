#pragma once

#include "ui_sizer.h"
#include "ui_stylesheet.h"
#include <memory>
#include <map>
#include <functional>

namespace Halley
{
	class HalleyAPI;
	class ConfigNode;
	class Resources;
	class I18N;
	class UIWidget;

	class UIFactory
	{
	public:
		using WidgetFactory = std::function<std::shared_ptr<UIWidget>(const ConfigNode&)>;

		UIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet);

		std::shared_ptr<UIWidget> makeUI(const ConfigNode& node);

	private:
		const HalleyAPI& api;
		Resources& resources;
		I18N& i18n;
		std::shared_ptr<UIStyleSheet> styleSheet;

		std::map<String, WidgetFactory> factories;

		std::shared_ptr<UIWidget> makeWidget(const ConfigNode& node);
		std::shared_ptr<UISizer> makeSizerPtr(const ConfigNode& node);
		Maybe<UISizer> makeSizer(const ConfigNode& node);
		void loadSizerChildren(UISizer& sizer, const ConfigNode& node);

		static Vector2f asVector2f(const ConfigNode& node, Maybe<Vector2f> defaultValue);
		static Vector4f asVector4f(const ConfigNode& node, Maybe<Vector4f> defaultValue);
		LocalisedString parseLabel(const ConfigNode& node);

		std::shared_ptr<UIWidget> makeBaseWidget(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeLabel(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeButton(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeTextInput(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeList(const ConfigNode& node);
	};
}
