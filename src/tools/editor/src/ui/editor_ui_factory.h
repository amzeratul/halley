#pragma once
#include "project_window.h"
#include "halley/ui/ui_factory.h"

namespace Halley {
    class EditorUIFactory : public UIFactory {
    public:
		EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, const String& colourSchemeName);

    	Sprite makeAssetTypeIcon(AssetType type) const override;
    	Sprite makeImportAssetTypeIcon(ImportAssetType type) const;
    	Sprite makeDirectoryIcon(bool up = false) const;

    	Vector<String> getColourSchemeNames() const;
    	void setColourScheme(const String& name);

        void setProject(ProjectWindow* projectWindow, Resources* gameResources);

	private:
		std::shared_ptr<UIWidget> makeScrollBackground(const ConfigNode& node);
        std::shared_ptr<UIWidget> makeInfiniCanvas(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeAnimationEditorDisplay(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeMetadataEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeSceneEditorCanvas(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityList(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityValidator(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityValidatorList(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeSelectAsset(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeUIWidgetEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeUIWidgetList(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeUIEditorDisplay(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeAudioObjectTreeList(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeCurveEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeGradientEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeColourPickerDisplay(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeScriptingVariableInspector(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeMeshEditorDisplay(const ConfigNode& entryNode);

		UIFactoryWidgetProperties getCurveEditorProperties() const;
		UIFactoryWidgetProperties getSelectAssetProperties() const;

    	void loadColourSchemes();
    	void reloadStyleSheet();

    	void setColourSchemeByAssetId(const String& assetId);
        
    	Vector<std::pair<String, String>> colourSchemes;
        ProjectWindow* projectWindow = nullptr;
        Resources* gameResources = nullptr;
    };
}
