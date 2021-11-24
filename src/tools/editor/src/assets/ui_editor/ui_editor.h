#pragma once

#include <memory>
#include <memory>

#include "ui_widget_editor.h"
#include "ui_widget_list.h"
#include "../asset_editor.h"
#include "src/scene/choose_asset_window.h"

namespace Halley {
	class UIGraphNode;

	class UIEditor : public AssetEditor {
	public:
		UIEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow, const HalleyAPI& api);

		void reload() override;
		void onMakeUI() override;
		void markModified();
		void onWidgetModified(const String& id);

		bool isModified() override;
		void save() override;

		UIFactory& getGameFactory();

		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
		std::unique_ptr<UIFactory> gameFactory;
		std::unique_ptr<I18N> gameI18N;
		std::shared_ptr<UIWidgetList> widgetList;
		std::shared_ptr<UIWidgetEditor> widgetEditor;
		ProjectWindow& projectWindow;

		std::shared_ptr<UIDefinition> uiDefinition;
		std::shared_ptr<UIWidget> display;
		bool loaded = false;
		bool modified = false;

		String curSelection;

		void doLoadUI();
		void setSelectedWidget(const String& id);

		void addWidget();
		void addWidget(const String& widgetClass);
		void addWidget(const String& referenceId, bool asChild, ConfigNode data);
		void removeWidget();
		void removeWidget(const String& id);
	};

	class ChooseUIWidgetWindow : public ChooseAssetWindow {
	public:
		ChooseUIWidgetWindow(UIFactory& factory, UIFactory& gameFactory, Callback callback);

	protected:
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch) override;

	private:
		UIFactory& factory;
		UIFactory& gameFactory;
	};
}
