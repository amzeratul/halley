#pragma once

#include "../asset_editor.h"
#include "src/ui/scroll_background.h"

namespace Halley {
    class ScriptGraphEditor : public AssetEditor {
	public:
		ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;
		void onMakeUI() override;
		
		void save() override;
		bool isModified() override;
		void markModified();
		
	protected:
    	void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

    private:
		ProjectWindow& projectWindow;
    	std::shared_ptr<ScriptGraph> scriptGraph;

    	std::shared_ptr<ScrollBackground> scrollBg;
		bool modified = false;
    };
}
