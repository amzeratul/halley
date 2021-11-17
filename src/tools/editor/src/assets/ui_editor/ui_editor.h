#pragma once

#include <memory>
#include <memory>

#include "ui_widget_editor.h"
#include "ui_widget_list.h"
#include "../asset_editor.h"

namespace Halley {
	class UIGraphNode;

	class UIEditor : public AssetEditor {
	public:
		UIEditor(UIFactory& factory, Resources& gameResources, Project& project, const HalleyAPI& api);

		void reload() override;
		void onMakeUI() override;
		void onWidgetModified();

	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
		std::unique_ptr<UIFactory> gameFactory;
		std::unique_ptr<I18N> gameI18N;
		std::shared_ptr<UIWidgetList> widgetList;
		std::shared_ptr<UIWidgetEditor> widgetEditor;

		std::shared_ptr<UIDefinition> uiDefinition;
		std::shared_ptr<UIWidget> display;
		bool loaded = false;

		void doLoadUI();
		void setSelectedWidget(const String& id);
	};
}
