#pragma once

#include <halley.hpp>
#include "ui_widget_editor.h"
#include "ui_widget_list.h"
#include "../asset_editor.h"
#include "halley/tools/dll/project_dll.h"
#include "src/scene/choose_window.h"
#include "src/ui/infini_canvas.h"

namespace Halley {
	class UIEditorDisplay;
	class UIGraphNode;

	class UIEditor : public AssetEditor, IProjectDLLListener {
	public:
		UIEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow, const HalleyAPI& api);
		~UIEditor() override;

		void update(Time t, bool moved) override;

		void reload() override;
		void onMakeUI() override;
		void markModified();
		void onWidgetModified(const String& id);
		void reselectWidget();

		bool isModified() override;
		void save() override;

		UIFactory& getGameFactory();

		bool onKeyPress(KeyboardKeyPress key) override;

        void copyWidgets(const Vector<String>& ids);
        void cutWidgets(const Vector<String>& ids);
        void pasteAt(const String& referenceId, bool asChild);
        void addWidgetsAt(const String& referenceId, bool asChild, Vector<ConfigNode> datas);
        void deleteWidgets(const Vector<String>& ids);

	protected:
		void onProjectDLLStatusChange(ProjectDLL::Status status) override;
		
	private:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
		std::unique_ptr<UIFactory> gameFactory;
		std::unique_ptr<I18N> gameI18N;
		std::shared_ptr<UIWidgetList> widgetList;
		std::shared_ptr<UIWidgetEditor> widgetEditor;
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		ProjectWindow& projectWindow;
		const HalleyAPI& api;

		std::shared_ptr<UIDefinition> uiDefinition;
		std::shared_ptr<UIEditorDisplay> display;
		bool loaded = false;
		bool modified = false;
		bool pendingLoad = false;
		bool firstLoad = true;

		String curSelection;

		void open();
		void doLoadUI();
		void setSelectedWidget(const String& id);

		void addWidget();
		void addWidget(const String& widgetClass);
		void removeWidget();

		void loadGameFactory();

		void reassignUUIDs(ConfigNode& node) const;
	};

	class ChooseUIWidgetWindow : public ChooseAssetWindow {
	public:
		ChooseUIWidgetWindow(UIFactory& factory, UIFactory& gameFactory, Callback callback);

	protected:
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch) override;
		void sortItems(Vector<std::pair<String, String>>& items) override;

		int getNumColumns(Vector2f scrollPaneSize) const override;

	private:
		UIFactory& factory;
		UIFactory& gameFactory;
	};
}
