#pragma once

#include "halley/text/halleystring.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/text/i18n.h"
#include <vector>
#include <memory>

namespace Halley {
    class UIFactory;
	class UIWidget;
    class Project;
    class Resources;
    class HalleyAPI;
	class IYAMLConverter;
    class Path;
	
    class IEditorCustomTools { 
    public:
        struct ToolData {
            String id;
            LocalisedString text;
        	LocalisedString tooltip;
            Sprite icon;
            std::shared_ptr<UIWidget> widget;

        	ToolData(String id, LocalisedString text, LocalisedString tooltip, Sprite icon, std::shared_ptr<UIWidget> widget)
                : id(std::move(id))
                , text(std::move(text))
        		, tooltip(std::move(tooltip))
                , icon(std::move(icon))
                , widget(std::move(widget))
            {}
        };

        struct MakeToolArgs {
        	UIFactory& factory;
            Resources& editorResources;
            Resources& gameResources;
            const HalleyAPI& api;
            const std::shared_ptr <IYAMLConverter> yamlConverter = nullptr;

            MakeToolArgs(UIFactory& factory, Resources& editorResources, Resources& gameResources, const HalleyAPI& api, const std::shared_ptr<IYAMLConverter> yamlConverter)
                : factory(factory)
                , editorResources(editorResources)
        		, gameResources(gameResources)
                , api(api)
        		, yamlConverter(yamlConverter)
            {}
        };

        virtual ~IEditorCustomTools() = default;

        virtual std::vector<ToolData> makeTools(const MakeToolArgs& args) = 0;
    };

	class IYAMLConverter {
    public:		
        virtual ~IYAMLConverter() = default;
        virtual String generateYAML(const ConfigNode& config) const = 0;
        virtual void writeYAMLAsset(const ConfigNode& config, const Path& relativePath) const = 0;
	};
}
