#pragma once
#include "halley/ui/ui_factory.h"

namespace Halley {
    class EditorUIFactory : public UIFactory {
    public:
		EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n);

    	Sprite makeAssetTypeIcon(AssetType type) const;
    	Sprite makeImportAssetTypeIcon(ImportAssetType type) const;
    	Sprite makeDirectoryIcon(bool up = false) const;

    	std::vector<String> getColourSchemeNames() const;
    	void setColourScheme(const String& name);

    private:
		std::shared_ptr<UIWidget> makeScrollBackground(const ConfigNode& node);
		std::shared_ptr<UIWidget> makeAnimationEditorDisplay(const ConfigNode& entryNode);
		std::shared_ptr<UIWidget> makeMetadataEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeSceneEditorCanvas(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityList(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeEntityEditor(const ConfigNode& entryNode);
        std::shared_ptr<UIWidget> makeSelectAsset(const ConfigNode& entryNode);

    	void loadColourSchemes();
    	void reloadStyleSheet();

    	std::vector<std::shared_ptr<UIColourScheme>> colourSchemes;
    };
}
