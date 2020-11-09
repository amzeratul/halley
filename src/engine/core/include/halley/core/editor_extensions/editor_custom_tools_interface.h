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
    class Path;

	class IProject {
    public:		
        virtual ~IProject() = default;
		virtual Path getAssetsSrcPath() const = 0;
	};

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
        	IProject& project;

            MakeToolArgs(UIFactory& factory, Resources& editorResources, Resources& gameResources, const HalleyAPI& api, IProject& project)
                : factory(factory)
                , editorResources(editorResources)
        		, gameResources(gameResources)
                , api(api)
        		, project(project)
            {}
        };

        virtual ~IEditorCustomTools() = default;

        virtual std::vector<ToolData> makeTools(const MakeToolArgs& args) = 0;
    };
}
